# PS3GL
PS3GL is a WIP OpenGL 1.X Like API on top of the PSL1GHT RSX API  
  
https://github.com/user-attachments/assets/5fad3b1b-b056-4624-888f-c325e55272db  

NOTE: The ABI of ps3glInit isn't stable yet 

# TODO
- Make RGB -> DRGB and RGBA -> ARGB conversion faster (Maybe using Altivec?)
- Implement glMatrixPush/glMatrixPop
- Implement Lights
- Implement Fog
- Implement Textures (WIP)
- Make code less bad

# Credits
Fancy2209 - Author  
kd-11 - Help with GCM and the RSX in general  
Rinnegatamente - Told how to calculate the Model View Projection Matrix in a faster manner and made vitaGL, who inspired this project  