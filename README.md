# xmodelconverter
A converter for xmodel & xanim files for Call of Duty.

It converts xmodel & xanim files back into xmodel_export and xanim_export text readable files.
These _export files can then be more easily read and imported into 3d modelling software such as Blender (https://blender.org).

## Supported games
- ~~Call of Duty~~ (not available at this moment)
- Call of Duty 2

## Usage

Create a new folder in your main folder, so that your structure looks like.

```
\exported
\xanim
\xmodel
\xmodelparts
\xmodelsurfs
```

**Then you can just drop and drop the xmodel or xanim file(s) onto the executable.**

or use

```
xmodelconverter.exe "C:\path\to\your\xmodel\or\xanim\file"
```

**The files will be placed in the exported folder.**

## Dependencies

https://github.com/g-truc/glm

## Building

```
sudo apt install libglm-dev
git clone https://github.com/riicchhaarrd/xmodelconverter
g++ -w *.cpp -o xmodelconverter
```

## Blender
To import the _export files into Blender, you need to install a addon.

https://github.com/riicchhaarrd/io_scene_xmodel
