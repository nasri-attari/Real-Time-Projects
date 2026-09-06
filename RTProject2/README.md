# Restaurant Supply Chain Simulation



A real-time, multi-process simulation of a restaurant workflow—from supply chain and food preparation to customer service and sales.



## System roles


- **Manager** — starts and coordinates the simulation

- **Supply Chain** — provides ingredients and resources

- **Baker** — prepares baked items

- **Chef** — prepares meals

- **Seller** — handles sales

- **Customer** — generates customer requests

- **GUI** — displays the simulation state



## Technical highlights



- C-based multi-process architecture

- Separate executables for each system role

- System V IPC resources: shared memory, message queues, and semaphores

- Configurable behavior through `config.txt`

- OpenGL/GLUT graphical interface

- Docker files included for containerized development



## Requirements



- Linux or WSL

- GCC

- Make

- OpenGL and GLUT development libraries



## Build and run



```bash

cd RTProject2

make

make run

