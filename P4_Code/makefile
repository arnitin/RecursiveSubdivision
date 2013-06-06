all :
	g++ glm.c -c -o glm.obj -lGL -lGLU -lglut -nostartfiles -lm
	g++ gltb.c -c -o gltb.obj -lGL -lGLU -lglut -nostartfiles -lm
	g++ smooth.c -o loop glm.obj gltb.obj -lGL -lGLU -lglut -g

clean :
	 rm -rf *o *out loop