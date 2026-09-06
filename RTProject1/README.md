# Tug-of-War Real-Time Simulation



A configurable real-time systems simulation of a tug-of-war match between two teams. The system models players, a referee, match rules, energy recovery, round scoring, and a graphical interface.



## Highlights



- Two teams with four player processes each

- A referee process that controls rounds, scoring, timing, and win conditions

- POSIX process management using `fork()` and `exec()`

- Inter-process communication through named pipes (FIFOs)

- Signal-based player energy updates and recovery events

- Configurable simulation behavior through `config.txt`

- OpenGL/GLUT graphical interface for displaying match state



\## Architecture



```text

simulation

├── referee

│   └── player processes (two teams)

└── GUI



referee → FIFO → GUI

