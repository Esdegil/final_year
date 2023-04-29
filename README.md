# Final Year Project. Interactive Chess Board. Vladislavas Putys {#readme_link}

# version number v1.2

## v1.2 Impossible moves not allowed, Documentation can be generated with the used of doxygen.
## v1.1 Improved UX by better Display and LED Service integration across the codebase.
## v1.0 Minimal requirements complete. Implemented game engine with some things still left to do.
## v0.2-0.3 Minimal requirements almost complete. Input matrix is connected with chess engine and led service.

## The plan is to merge dev to master once in a week with following notation vX.Y
### Y will be changed weekly and X should be changed with major updates such as finishing minimal requirements, etc.

## Documentation

to get documetation run `doxygen Doxyfile` from the root directory.

## Instructions to run everything (in case I forget)
After finishing changes to the image do (from root dir in terminal)
``` 
idf.py build 
idf.py flash monitor
```
Note: if flash monitor doesn't work because of missing serial - check cable OR do `sudo chmod -R 777 /dev/ttyUSB0` \
Note: if idf.py build can't find idf.py run `. $HOME/esp/esp-idf/export.sh`

# NOTE ON CMakeLists
if a component needs another local component it needs to be included as `REQUIRES <component_name>`.\
If a component needs another system level component (like driver/gpio) you need to find original name and do the same as above. \
If a component needs some code from the same level it needs to be added in SRCS and INCLUDE.
Better add new componends with idf.py add-component <>

// TODO: Change these terminal instructions to be something faster usable.

# Usable pins (tested)
Input/output: 32, 33, 25, 26, 27, 23, 22, 21, 19, 18, 12 (needs gpio_reset_pin), 13 (needs gpio_reset_pin), 14 (needs gpio_reset_pin), 16 (looks like it doesn't need anything else), 17 (looks like it doesn't need anything else), 4 (looks like it doesn't need anything else) \
Input only: 34, 35, 39 (VN), 36 (VP) \
Potentially usable: 5, 15, 2

# Important TODOs and known problems
### King vs king problem
happens when the only figures on the board are 2 kings. Because of how `check_all_enemy_moves_for_pos()` function works.
### Requires castle option. (cannot be tested without 8x8 board)
Not sure if can be done before moving to 8x8 board.
### King not being able to move out of the check (solved in 1.0)
Note: looks like I've fixed it, but it's poorly tested. Might be really buggy