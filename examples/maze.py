# Maze generator

import sys

sys.path.append('lib')

import random

WALL = 0
FLOOR = 1
GRAVE = 2
PORTAL = 3
MAGIC_TORCH = 4
START = 5

MAX_GRAVES = 5
MAX_PORTALS = 2
MAX_MAGIC_TORCH = MAX_PORTALS + 1

def make_row(width):
    row = []
    x = 0

    while x < width:
        row.append(WALL)
        x = x + 1

    return row


def make_maze_array(width, height):
    maze = []
    y = 0

    while y < height:
        maze.append(make_row(width))
        y = y + 1

    return maze


def generate_maze(width, height):
    maze = make_maze_array(width, height)

    # Start at (1, 1).
    maze[1][1] = FLOOR

    # Explicit stacks instead of recursive function calls.
    stack_x = [1]
    stack_y = [1]

    while len(stack_x) > 0:
        x = stack_x[len(stack_x) - 1]
        y = stack_y[len(stack_y) - 1]

        # Maximum four possible neighbors.
        nx = []
        ny = []

        # North
        if y > 2:
            if maze[y - 2][x] == WALL:
                nx.append(x)
                ny.append(y - 2)

        # East
        if x + 2 < width - 1:
            if maze[y][x + 2] == WALL:
                nx.append(x + 2)
                ny.append(y)

        # South
        if y + 2 < height - 1:
            if maze[y + 2][x] == WALL:
                nx.append(x)
                ny.append(y + 2)

        # West
        if x > 2:
            if maze[y][x - 2] == WALL:
                nx.append(x - 2)
                ny.append(y)

        count = len(nx)

        if count == 0:
            stack_x.pop()
            stack_y.pop()

        else:
            i = random.randint(0, count - 1)

            new_x = nx[i]
            new_y = ny[i]

            # Remove wall between the cells.
            wall_x = (x + new_x) // 2
            wall_y = (y + new_y) // 2

            maze[wall_y][wall_x] = FLOOR
            maze[new_y][new_x] = FLOOR

            stack_x.append(new_x)
            stack_y.append(new_y)

    return maze

def generate_graves_and_portals(maze):
    magic_torch_placed = 0
    width = len(maze[0])
    height = len(maze)

    graves_placed = 0
    portals_placed = 0

    while graves_placed < MAX_GRAVES:
        if magic_torch_placed < MAX_MAGIC_TORCH:
            x = random.randint(1, width - 2)
            y = random.randint(1, height - 2)

            if maze[y][x] == FLOOR:
                maze[y][x] = MAGIC_TORCH
                magic_torch_placed += 1
        x = random.randint(1, width - 2)
        y = random.randint(1, height - 2)

        if maze[y][x] == FLOOR:
            maze[y][x] = GRAVE
            graves_placed += 1

    while portals_placed < MAX_PORTALS:
        x = random.randint(1, width - 2)
        y = random.randint(1, height - 2)

        if maze[y][x] == FLOOR:
            maze[y][x] = PORTAL
            portals_placed += 1

    return maze

def select_start_position(maze):
    width = len(maze[0])
    height = len(maze)

    while True:
        x = random.randint(1, width - 2)
        y = random.randint(1, height - 2)

        if maze[y][x] == FLOOR:
            return x, y
    
def place_start_position(maze):
    x, y = select_start_position(maze)
    if x is not None and y is not None:
        maze[y][x] = START
    return maze

def save_to_file(maze, filename):
    f = fopen(filename, "w")
    y = 0

    while y < len(maze):
        x = 0
        line = ""

        while x < len(maze[y]):
            if maze[y][x] == WALL:
                line = line + "#"
            elif maze[y][x] == GRAVE:
                line = line + "G"
            elif maze[y][x] == PORTAL:
                line = line + "P"
            elif maze[y][x] == MAGIC_TORCH:
                line = line + "M"
            elif maze[y][x] == START:
                line = line + "S"
            else:
                line = line + "."

            x = x + 1

        fwrite(f, line + "\n")
        y = y + 1

    fclose(f)
    print("Maze saved to " + filename)

def print_maze(maze):
    y = 0

    while y < len(maze):
        x = 0
        line = ""

        while x < len(maze[y]):
            if maze[y][x] == WALL:
                line = line + "#"
            elif maze[y][x] == GRAVE:
                line = line + "G"
            elif maze[y][x] == PORTAL:
                line = line + "P"
            elif maze[y][x] == MAGIC_TORCH:
                line = line + "M"
            elif maze[y][x] == START:
                line = line + "S"
            else:
                line = line + "."

            x = x + 1

        print(line)

        y = y + 1

random.seed(time_tick())
width = 20
height = 11
maze = generate_maze(width, height)
maze = generate_graves_and_portals(maze)
maze = place_start_position(maze)
print_maze(maze)
save_to_file(maze, "maze.txt")
