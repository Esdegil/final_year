# final year project Vlad

# version number v0.1

## The plan is to merge dev to master once in a week with following notation vX.Y
### Y will be changed weekly and X should be changed with major updates such as finishing minimal requirements, etc.

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
