Extract files by enabling texture dumping in the config. Alternatively something like Replicant2Blender (https://github.com/WoefulWolf/Replicant2Blender) may be more useful. Then change them however you want and save them as dds images. New textures must have the same resolution, mipmap count, and compression format as the original. The last two can be found out by enabling File Info logging. Textures should be placed in the root folder of the mod. There are two ways to match:

- Filename (e.g. "nier029_bodys_mt_n.dds"). This can be found using texture dump in Lunar Tear (enable in config)
- CRC32C LOD0 Hash (e.g. "546DD78C.dds"). This is how SpecialK's texture injection matches textures, supported in this for compatibility.
