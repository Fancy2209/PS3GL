# PS3GL
PS3GL is a WIP OpenGL 1.X Like API on top of the PSL1GHT RSX API  
  
https://github.com/user-attachments/assets/5fad3b1b-b056-4624-888f-c325e55272db  

NOTE: The ABI of ps3glInit isn't stable yet 

# TODO
- Make RGB -> XRGB conversion faster (Maybe using Altivec?)
- Implement Lights
- Implement Fog (Added, not sure if it works though)
- Implement more Texture Formats and stop assuming RGBA Everywhere
- NV_vertex_program3/NV_fragment_program2 support using the PSL1GHT CGComp Source Code.  

# Credits
Fancy2209 - Author  
kd-11 - Help with GCM and the RSX in general  
Rinnegatamente - Told how to calculate the Model View Projection Matrix in a faster manner and made vitaGL, who inspired this project  
