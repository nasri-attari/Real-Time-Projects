\# Concurrent Police-and-Gang Simulation



A configurable real-time simulation in which independent gang and police processes interact while a graphical interface displays the system state.



The project demonstrates coordination between multiple Linux processes and ends when configured thresholds for successful plans, thwarted plans, or exposed agents are reached.



\## Components



\- \*\*Main process\*\* — initializes shared resources and controls simulation shutdown

\- \*\*Gang processes\*\* — simulate independent gangs and their plans

\- \*\*Police process\*\* — responds to gang activity

\- \*\*GUI process\*\* — visualizes the simulation

\- \*\*Configuration module\*\* — loads parameters from `config.txt`



\## Technical highlights



\- Process creation and execution with `fork()` and `exec()`

\- Shared memory for common simulation state

\- System V semaphores for synchronization

\- System V message queues for process communication

\- Signals for process lifecycle control

\- OpenGL/GLUT GUI

\- Configurable simulation limits



\## Architecture



```text

main

├── gang processes

├── police process

└── GUI process



Shared memory + semaphores + message queue

