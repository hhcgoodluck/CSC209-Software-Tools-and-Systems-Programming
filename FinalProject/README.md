# 5.1 Project Overview

This project implements a simulation of a RAID-4 storage system at the block level. The system is designed to provide fault tolerance and data reliability by distributing data blocks across multiple disks and maintaining a dedicated parity disk. In this simulation, each disk is represented by a separate child process, while a central controller (the parent process) coordinates all operations.

The main program (`raid_sim.c`) provides an interface for users to interact with the RAID system. It supports both an interactive shell-like interface and a transaction file interface, allowing users to issue commands either manually or through predefined input files. The system operates on fixed-size data blocks, and all operations are performed at the block level rather than at the file system level.

The RAID simulator supports the following core operations:
- **Write Block (`wb <block_num> <filename>`)**: Reads `block_size` bytes from a local file and writes the data to the specified logical block in the RAID system. This operation also updates the parity block to maintain consistency.
- **Read Block (`rb <block_num>`)**: Retrieves the data stored in a given logical block and prints it to standard output.
- **Simulate Disk Failure (`kill <disk_num>`)**: Sends a `SIGINT` signal to terminate a disk process, simulating a disk failure scenario.
- **Exit (`exit`)**: Sends a checkpoint command to all disk processes, causing them to write their data to disk files and terminate gracefully.

Internally, the system uses inter-process communication (IPC) via pipes to enable communication between the controller and disk processes. Each disk process continuously listens for commands (e.g., read, write, exit), executes them, and returns results when necessary. The controller is responsible for translating high-level RAID operations into low-level disk commands and ensuring proper coordination between processes.

A key feature of this system is its ability to tolerate disk failures. When a disk fails, the controller can reconstruct the lost data using the parity information and data from the remaining disks. This demonstrates the fundamental principle of RAID-4 systems, where redundancy is used to ensure data reliability.

Overall, this project belongs to the category of systems programming and demonstrates key concepts such as process creation (`fork`), inter-process communication (pipes), fault tolerance, and low-level data management.

Additionally, a graphical user interface (GUI.py) is provided to visualize the RAID system state and user interactions.





























# 5.4 Communication Protocol

Communication between the RAID controller (parent process) and each disk process (child process) is implemented using two dedicated pipes per disk.
For disk `i`, the controller sends requests through `controllers[i].to_disk[1]` and receives responses from `controllers[i].from_disk[0]`, while the child uses the corresponding opposite ends.

The system supports three disk-level commands: `CMD_READ`, `CMD_WRITE`, and `CMD_EXIT`.
These commands are issued by the controller in response to higher-level RAID operations such as `rb`, `wb`, `kill`, and `exit`.

## 5.4.1 Message Type: Read Request

| Field | Description |
|---|---|
| **Sender and receiver** | Parent (controller) → child disk process |
| **Encoding** | Two fixed-width binary values are written in sequence: first a `disk_command_t` with value `CMD_READ`, then an `int` containing the block number **within that disk**. |
| **Semantics / invariants** | The controller converts a logical RAID block number into a target disk and an in-disk block index before sending the request. For a data read, the target disk is `block_num % num_disks`; for a parity read, the target disk is `num_disks`. In both cases, the block number sent to the child is `block_num / num_disks`, i.e., the stripe index. The child must therefore interpret the message as: “read block `disk_block_num` from your local disk image and return exactly `block_size` bytes.” |
| **Response** | Child disk process → parent. The child sends back exactly `block_size` bytes containing the requested block data. The parent repeatedly calls `read()` until all `block_size` bytes have been received. |
| **Error handling** | If either write of the request fails with `-1`, the controller treats this as disk failure, calls `restore_disk_process(disk_num)`, and returns failure for the current operation. If the response cannot be read completely (for example, `read()` returns `<= 0` before `block_size` bytes are collected), the operation also fails. |

In the implementation, the controller does **not** send a variable-length message or a packed struct. 
Instead, it sends the message as a small protocol sequence: command opcode first, then block number, then waits for a fixed-size reply. 
This makes it clear to the receiver how many bytes to read and in what order.

## 5.4.2 Message Type: Write Request

| Field | Description |
|---|---|
| **Sender and receiver** | Parent (controller) → child disk process |
| **Encoding** | Three pieces of data are written in sequence: (1) a `disk_command_t` with value `CMD_WRITE`, (2) an `int` containing the block number within that disk, and (3) exactly `block_size` bytes of payload data. |
| **Semantics / invariants** | As with reads, the controller first maps the logical RAID block to a specific disk and stripe index. The child must read the command, then read the block number, then read exactly `block_size` bytes and store them in the corresponding block slot of its local disk array. The protocol is position-based: after receiving `CMD_WRITE`, the child knows it must read one integer followed by one full block of bytes. |
| **Response** | No explicit acknowledgment message is sent back on success. Successful completion is inferred from the fact that all writes to the pipe succeed and the child remains alive. |
| **Error handling** | If any write operation (command, block number, or payload) returns -1, the controller treats this as a disk failure, invokes `restore_disk_process(disk_num)`, and aborts the current write operation. Short writes or zero-byte progress are also treated as errors. For the block payload, the controller uses a loop to ensure that all `block_size` bytes are eventually written.

This message type is used both for ordinary data writes and for parity writes. 
Importantly, the disk process itself is unaware of RAID semantics: parity computation is done entirely in the controller, 
which first reads the old data block and old parity block, computes `new_parity = old_parity XOR old_data XOR new_data`, 
writes the updated parity block, and then writes the new data block. 
Thus, the protocol seen by the child remains a simple block-write protocol.

## 5.4.3 Message Type: Exit / Checkpoint Request

| Field | Description |
|---|---|
| **Sender and receiver** | Parent (controller) → child disk process |
| **Encoding** | A single fixed-width binary value of type `disk_command_t` with value `CMD_EXIT`. No additional fields follow. |
| **Semantics / invariants** | After reading `CMD_EXIT`, the disk process must checkpoint its disk contents to a file (such as `disk_X.dat`) and then terminate. Since the command contains no payload, the receiver knows that the message ends immediately after the command value. |
| **Response** | No explicit data response is required. The parent observes termination by waiting for child processes using `wait()`. |
| **Error handling** | In `checkpoint_and_wait()`, if the parent cannot successfully send `CMD_EXIT` to a disk, it prints a warning and continues shutdown. Since this is a final cleanup phase, the implementation treats these failures as non-fatal. |

This matches the specification that the user-level `exit` command causes the controller to send checkpoint commands to all disks 
and then wait for the child processes to terminate.

## 5.4.4 Failure-Triggered Recovery as Part of the Protocol

Although disk recovery is not encoded as a separate pipe message type, it is an integral part of the communication protocol.
In this design, communication failure is used as the mechanism for detecting disk failure.

The controller explicitly ignores `SIGPIPE`, ensuring that a failed disk process does not terminate the entire system.
Instead, when a `write()` to a disk pipe returns `-1`, the controller interprets this as a disk failure. It then closes the corresponding pipe endpoints,
restarts the disk process using `restart_disk()`, and reconstructs the missing data.

Recovery is performed stripe by stripe using the parity relation. For a failed data disk, each missing block is reconstructed by XORing
the parity block with all remaining data blocks in the same stripe. For a failed parity disk, each parity block is recomputed by XORing
all data blocks in the stripe. This recovery logic is implemented in `restore_disk_process()`.

## 5.4.5 Summary

Overall, the protocol is a compact, opcode-driven binary protocol over pipes.
Each message is self-delimiting, as the command opcode determines both the structure and length of the remaining fields.
This design simplifies the disk processes, enables natural synchronization through blocking I/O,
and tightly integrates failure detection into the communication layer.











# 5.5 Concurrency Model

This project follows the Category 1 multi-process model using pipes, where a parent process coordinates multiple worker processes executing concurrently.

The parent process, implemented in `controller.c`, acts as the central controller of the RAID system. It is responsible for initializing all disk processes using `fork()`, managing communication, and coordinating read and write operations across disks.

Each disk in the RAID system is implemented as a separate child process (worker). These disk processes are created during system initialization and remain active throughout execution. At runtime, multiple disk processes operate concurrently, satisfying the requirement that at least three worker processes run simultaneously.

Communication between the controller and disk processes is implemented using pipes. For each disk process, two pipes are established: one for sending commands and data from the controller to the disk, and one for receiving responses from the disk. This design ensures that all inter-process communication occurs through pipes, as required.

Pipes also serve as a synchronization mechanism. When a disk process calls `read()` on a pipe, it blocks until data is available, ensuring that commands are processed in the correct order. Similarly, the controller may block while waiting for responses from disk processes. This implicit synchronization avoids the need for additional coordination mechanisms such as shared memory or explicit locks.

Overall, this design achieves concurrent execution of multiple worker processes, coordinated by a single parent process, with all communication and synchronization handled through pipes.





# 5.6 Error Handling and Robustness

Our implementation includes explicit error checking for system calls and pipe-based communication. Since the RAID simulator is built as a multi-process application using pipes, many runtime failures can occur at the operating-system level. The controller handles these failures by checking return values, printing diagnostic messages with `perror()` or `fprintf(stderr, ...)`, closing invalid file descriptors when necessary, and returning `-1` so that the caller can stop or recover safely.

Below are several examples of bad runtime behaviours and how the code handles them.

## 5.6.1. Pipe creation failure

**Bad behaviour:**  
A call to `pipe()` may fail while the controller is initializing communication channels for a disk process.

**How the code handles it:**  
The return value of `pipe()` is checked immediately. If pipe creation fails, the code reports the error using `perror()`. If one pipe has already been created successfully before the second one fails, the already-open file descriptors are closed before returning. This prevents file descriptor leaks and ensures that initialization does not continue in a partially constructed state.

**Why this is robust:**  
Without this check, later reads and writes would use invalid file descriptors and lead to undefined behaviour. By failing early and cleaning up immediately, the controller preserves a consistent state.

## 5.6.2. Fork failure when creating a disk process

**Bad behaviour:**  
A call to `fork()` may fail when the controller attempts to create a child disk process.

**How the code handles it:**  
The controller checks whether `fork()` returns a negative value. If it does, the code reports the error with `perror()`, closes all pipe file descriptors that were opened for that disk, and returns failure instead of continuing.

**Why this is robust:**  
This prevents the controller from incorrectly assuming that a worker exists when it does not. It also avoids leaking pipe descriptors associated with a child process that was never created.

## 5.6.3. Writing to a closed or broken pipe

**Bad behaviour:**  
If a disk process has terminated, any later `write()` by the controller to that disk’s pipe may fail because the read end no longer exists.

**How the code handles it:**  
The program ignores `SIGPIPE` so that the controller itself is not killed automatically by the operating system. Instead, it checks whether `write()` returns `-1`. When this happens, the code treats it as a disk communication failure, reports the problem, and stops the current pipe operation safely. In the disk recovery path, this failure can also trigger restoration logic.

**Why this is robust:**  
Ignoring `SIGPIPE` and checking `write()` explicitly allows the controller to stay alive and handle the failure in code, rather than crashing unexpectedly.

## 5.6.4. Short read or failed read from a pipe

**Bad behaviour:**  
A `read()` from a pipe may fail, return `0`, or return fewer bytes than expected. This can happen if the child process exits early or if communication is interrupted.

**How the code handles it:**  
The controller does not assume that one `read()` call will always return a full block. It checks the return value of `read()` and uses loops when necessary to continue reading until the required number of bytes has been received. If `read()` returns `<= 0` before the full message is collected, the code treats the operation as a failure and returns an error.

**Why this is robust:**  
This prevents the controller from using incomplete block data or uninitialized memory. It also makes the protocol reliable even when pipe reads are fragmented.

## 5.6.5. Invalid block number supplied to controller functions

**Bad behaviour:**  
A caller may request a read or write using a logical block number outside the valid RAID range.

**How the code handles it:**  
The controller validates the block number before sending any request to a disk process. If the block number is invalid, the function immediately returns `-1` and does not send malformed commands through the pipe.

**Why this is robust:**  
This prevents out-of-range indexing, incorrect disk selection, and invalid offsets inside the child disk process. It also keeps the communication protocol well-formed.

## 5.6.6. Failure while shutting down disk processes

**Bad behaviour:**  
During shutdown, a disk may already be dead when the controller attempts to send `CMD_EXIT`, or waiting for child termination may encounter an already-collected or failed child.

**How the code handles it:**  
The controller still checks the return values of writes during checkpoint and shutdown. If a write fails, it reports the error and continues cleanup rather than aborting the whole shutdown sequence. It then calls `wait()` to collect child processes that can still be reaped.

**Why this is robust:**  
Shutdown code should be best-effort rather than fragile. Even if one disk is already gone, the controller still cleans up the remaining processes and file descriptors as safely as possible.

## 5.6 Conclusion
The main robustness strategy in this project is to treat every important system call as potentially fallible. In particular, the implementation checks the results of `pipe()`, `fork()`, `read()`, `write()`, and process cleanup operations. When an error occurs, the code reports it, closes any no-longer-needed file descriptors, avoids continuing in an inconsistent state, and returns an error code to the caller. This makes the simulator much more reliable as a multi-process system program.








# 5.7 Team Contributions
By myself






