set vert="0.vert"
set frag="0.frag"

glslangValidator -Os -V %vert% -o ./%vert%.spv
glslangValidator -Os -V %frag% -o ./%frag%.spv