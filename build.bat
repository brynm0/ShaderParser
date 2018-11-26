@echo off

set GLFWLibPath= "W:\glfw-3.2.1.bin.WIN64\lib-vc2015\glfw3.lib"
set VKLibPath= "W:\VulkanSDK\1.1.85.0\Lib\vulkan-1.lib"

set GLFWIncludePath= "W:\glfw-3.2.1.bin.WIN64\include"
set VKIncludePath= "W:\VulkanSDK\1.1.85.0\Include"
set BLibPath= "W:\Blib"
set commonCompilerFlags=  -nologo -Gm- -GR- -EHsc- -O2 -Oi -FC -Z7 -I %BLibPath% -I %GLFWIncludePath% -I %VKIncludePath% 
set commonLinkerFlags= -debug:fastlink -NODEFAULTLIB:msvcrt.lib -incremental:no -opt:ref user32.lib shell32.lib gdi32.lib winmm.lib 

REM 64 bit build
del *.pdb > NUL 2> NUL
cl -MDd %commonCompilerFlags% -Fmshader_parser.map W:\ShaderParser\ShaderParser.cpp /link %commonLinkerFlags% %GLFWLibPath% %VKLibPath% /PDB:shader_parser_%random%.pdb
