#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#include <GL/glew.h>
#include <GL/glut.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "file_loader.h"

//------------DEFINES
#define WIDTH 10
#define HEIGHT 23

//------------PROTOTYPES
void createBoard(int* board);
void printBoard(int* board);
void gameLoop();
void moveLeft();
void moveRight();
long get_time();
void changeGameSpeed();
void newFigure();
void delLine(int row);
void rotate();
void drawRect(glm::vec2, glm::vec2);
void compileShaders(char* vertexSource, char* fragmentSource);

long get_time()
{
    struct timespec ts;

    if (clock_gettime (CLOCK_MONOTONIC, &ts) == 0)
        return (long) (ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
    else
        return 0;
}

//------------DATA STRUCTURES
typedef struct
{
    int row;
    int column;
} vec2;

//------------FIGURES
int f0[] = 
{
     0,  0,  0,  1,
     0,  1,  2,  2,
     1,  1,  0, -1,
     0,  1,  1,  1,
    -1,  0,  0,  0,
     0,  0,  1,  2,
    -1,  0,  1, -1,
     1,  1,  1,  2
};

int f1[] = 
{
     0, 1, 0, 1,
     0, 0, 1, 1,
     0, 1, 0, 1,
     0, 0, 1, 1,
     0, 1, 0, 1,
     0, 0, 1, 1,
     0, 1, 0, 1,
     0, 0, 1, 1
};

int f2[] =
{
     0,  0,  0, -1,
     0,  1,  2,  2,
    -1, -1,  0,  1,
     0,  1,  1,  1,
     0,  1,  0,  0,
     0,  0,  1,  2,
    -1,  0,  1,  1,
     1,  1,  1,  2
};

int f3[] =
{
     0, -1,  0,  1,
     0,  1,  1,  1,
    -1, -1,  0, -1,
     1,  0,  0, -1,
    -1,  0,  1,  0,
    -1, -1, -1,  0,
     0,  1,  1,  1,
     0,  0, -1,  1
};

int f4[] = 
{
     0,  0,  0,  0,
    -2, -1,  0,  1,
    -1,  0,  1,  2,
     0,  0,  0,  0,
     0,  0,  0,  0,
    -2, -1,  0,  1,
    -1,  0,  1,  2,
     0,  0,  0,  0
};

int f5[] =
{
     0,  1, -1,  0,
     0,  0,  1,  1,
     0,  0,  1,  1,
    -1,  0,  0,  1,
     0,  1, -1,  0,
     0,  0,  1,  1,
     0,  0,  1,  1,
    -1,  0,  0,  1,
};

int f6[] = 
{
    -1,  0,  0,  1,
     0,  0,  1,  1,
     1,  0,  1,  0,
    -1,  0,  0,  1,
    -1,  0,  0,  1,
     0,  0,  1,  1,
     1,  0,  1,  0,
    -1,  0,  0,  1,
};




//------------GLOBAL VARIABLES
int*  board  = NULL;
vec2  origin = {2, 4};

int*  currentFigure = NULL;
int   currentState  = 0;

vec2  currentFigurePosition = {2, 4};

float timer        = 0;
float global_timer = 0;
float elapsedTime  = 0.1f;

int score   = 0;
int gameEnd = 0;

float gameSpeed = 1;

GLuint ProgramID;
GLuint VAO;

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitWindowSize(32*WIDTH, 32*HEIGHT);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutCreateWindow("Tetris!");

    if (glewInit() != GLEW_OK)
    {
        return -1;
    }

    char* vertexSource = load_file("shaders/transform.vert");
    char* fragmentSource = load_file("shaders/color.frag");

    compileShaders(vertexSource, fragmentSource);

    srand(time(0));
    board = (int*) malloc(sizeof(int) * WIDTH * HEIGHT);    
    createBoard(board);
    
    newFigure();

    glViewport(0, 0, 32*WIDTH, 32*HEIGHT);
    glm::mat4 Projection = glm::ortho(0.0f, 32.0f*WIDTH, 32.0f*HEIGHT, 0.0f, -1.0f, 1.0f);

    glutDisplayFunc(gameLoop);
    
    glutMainLoop();

    free(board);
    free(currentFigure);
    
    return 1;
}

void gameLoop()
{
    float currentTime = get_time(); 
    float dt = currentTime - elapsedTime;
    dt = dt / 1000000.0f;
    elapsedTime = currentTime;
    global_timer += dt;

    timer += dt;
    
    int isColliding = 0;
   
    changeGameSpeed();
    
    glClearColor(1.0f, 0.0f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //printBoard(board);
    //glFlush();

    drawRect(glm::vec2(200, 200), glm::vec2(300, 400));

    glutSwapBuffers();
    glFlush();

    if (timer > gameSpeed)
    {
        timer -= gameSpeed;
    
        if ((int)global_timer % 3 == 0)
        {
            rotate();
        }

        if (rand() % 2)
            moveLeft();
        else
            moveRight();
        

        for (int cell = 0; cell < 4; cell++)
        {
            //int x = figure[cell + (2*currentState) + 0) * cell];
            int cellRelativeRowPos = currentFigure[cell + ((2*currentState) + 1) * 4];
            int cellRelativeColPos = currentFigure[cell + ((2*currentState) + 0) * 4];
            
            int cellRow = cellRelativeRowPos + currentFigurePosition.row;
            int cellCol = cellRelativeColPos + currentFigurePosition.column;

            //ITS collision
            if (cellCol + (cellRow + 1) * WIDTH >= WIDTH * HEIGHT 
                    || board[cellCol + (cellRow + 1) * WIDTH] != 0)
            {
                isColliding = 1;   
            }        
        }
        if (isColliding)
        {
            for (int cell = 0; cell < 4; cell++)
            {
                int cellRelativeRowPos = currentFigure[cell + ((2*currentState) + 1) * 4];
                int cellRelativeColPos = currentFigure[cell + ((2*currentState) + 0) * 4];
                
                int cellRow = cellRelativeRowPos + currentFigurePosition.row;
                int cellCol = cellRelativeColPos + currentFigurePosition.column;
                board[cellCol + cellRow * WIDTH] = 1; 
            }

            if (currentFigurePosition.row == origin.row)
                gameEnd = 1;
            else
                score += 100;

            newFigure();
            currentFigurePosition.row    = origin.row;
            currentFigurePosition.column = origin.column;

            //check every if line full
            
            for (int row = 2; row < HEIGHT; row++)
            {
                int isFull = 1;
                for (int col = 0; col < WIDTH && isFull == 1; col++)
                {
                    if (board[col + row * WIDTH] != 1)
                        isFull = 0;
                }

                if (isFull)
                {
                    score += 1000;
                    delLine(row);              
                }

            }
        }
        else
        {
            currentFigurePosition.row++;
        }

    }

}

void createBoard(int* board)
{
    for (int i = 0; i < WIDTH * HEIGHT; i++)
        board[i] = 0;
}

void printBoard(int* board)
{
/*
    vec2 figure[4]; 
    for (int cell = 0; cell < 4; cell++)
    {
        int cellRelativeRowPos = currentFigure[cell + ((2*currentState) + 1) * 4];
        int cellRelativeColPos = currentFigure[cell + ((2*currentState) + 0) * 4];
        
        int cellRow = cellRelativeRowPos + currentFigurePosition.row;
        int cellCol = cellRelativeColPos + currentFigurePosition.column;
        figure[cell].column = cellCol;
        figure[cell].row    = cellRow;
    }


    for (int row = 2; row < HEIGHT; row++)
    {
        printf("| ");
        for (int col = 0; col < WIDTH; col++)
        {
            int isPrinted = 0;
            for (int cell = 0; cell < 4; cell++)
            {
                if (col == figure[cell].column && row == figure[cell].row)
                {
                    isPrinted = 1;
                    printf("▇ ");
                }

            }
            if (!isPrinted)
                printf("%s ", board[col + row * WIDTH] == 1 ? "▇" : ".");
        }
        printf("|\n");
    }
    printf("Time: %f\n", global_timer);
    printf("Score: %i\n", score);
    //printf("Game speed: %3.10f\n", gameSpeed);
*/
    vec2 figure[4]; 
    for (int cell = 0; cell < 4; cell++)
    {
        int cellRelativeRowPos = currentFigure[cell + ((2*currentState) + 1) * 4];
        int cellRelativeColPos = currentFigure[cell + ((2*currentState) + 0) * 4];
        
        int cellRow = cellRelativeRowPos + currentFigurePosition.row;
        int cellCol = cellRelativeColPos + currentFigurePosition.column;
        figure[cell].column = cellCol;
        figure[cell].row    = cellRow;
    }


    for (int row = 2; row < HEIGHT; row++)
    {
        for (int col = 0; col < WIDTH; col++)
        {
            if (board[col + row * WIDTH] == 1)
                glColor3f(1.0f, 1.0f, 1.0f);
            else
                glColor3f(0.0f, 0.0f, 0.0f);
            glBegin(GL_LINE_LOOP);
                glVertex2i(col*32, row*32);
                glVertex2i((col+1)*32, row*32);
                glVertex2i((col+1)*32, (row+1)*32);
                glVertex2i(col*32, (row+1)*32);
            glEnd();
        }
    }

    for (int cell = 0; cell < 4; cell++)
    {
        int col = figure[cell].column;
        int row = figure[cell].row;
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
            glVertex2i(col*32, row*32);
            glVertex2i((col+1)*32, row*32);
            glVertex2i((col+1)*32, (row+1)*32);
            glVertex2i(col*32, (row+1)*32);
        glEnd();
    }

    printf("Time: %f\n", global_timer);
    printf("Score: %i\n", score);
    //printf("Game speed: %3.10f\n", gameSpeed);

}

void changeGameSpeed()
{
    gameSpeed = -0.00831932f * global_timer + 1.00831933;
}

void moveLeft()
{
    int canMoveLeft = 1;
    for (int cell = 0; cell < 4; cell++)
    {
        int cellRelativeRowPos = currentFigure[cell + ((2*currentState) + 1) * 4];
        int cellRelativeColPos = currentFigure[cell + ((2*currentState) + 0) * 4];
        
        int cellRow = cellRelativeRowPos + currentFigurePosition.row;
        int cellCol = cellRelativeColPos + currentFigurePosition.column;
        if (cellCol == 0 || board[cellCol - 1 + cellRow * WIDTH] == 1)
            canMoveLeft = 0;
    }

    if (canMoveLeft)
    {
       currentFigurePosition.column--; 
    }
}

void moveRight()
{
    int canMoveRight = 1;
    for (int cell = 0; cell < 4; cell++)
    {
        int cellRelativeRowPos = currentFigure[cell + ((2*currentState) + 1) * 4];
        int cellRelativeColPos = currentFigure[cell + ((2*currentState) + 0) * 4];
        
        int cellRow = cellRelativeRowPos + currentFigurePosition.row;
        int cellCol = cellRelativeColPos + currentFigurePosition.column;

        if (cellCol == WIDTH - 1 || board[cellCol + 1 + cellRow * WIDTH] == 1)
            canMoveRight = 0;
    }

    if (canMoveRight)
    {
       currentFigurePosition.column++; 
    }
}


void newFigure()
{
    switch(rand() % 7)
    {
        case 0: currentFigure = f0; break;
        case 1: currentFigure = f1; break;
        case 2: currentFigure = f2; break;
        case 3: currentFigure = f3; break;
        case 4: currentFigure = f4; break;
        case 5: currentFigure = f5; break;
        case 6: currentFigure = f6; break;
    }
}

void delLine(int row)
{
    for (int i = row; i > 2; i--)
    for (int j = 0; j < WIDTH; j++)
        board[j + (i+1) * WIDTH] = board[j + (i) * WIDTH];
}

void rotate()
{
    currentState++;
    if (currentState > 3)
        currentState = 0;
}

void drawRect(glm::vec2 position, glm::vec2 size)
{
    //use shader
    glUseProgram(ProgramID);
    glm::mat4 model;
    model = glm::translate(model, glm::vec3(position, 0.0f));  

    model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f)); 
    model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f));

    model = glm::scale(model, glm::vec3(size, 1.0f)); 
  
    glUniformMatrix4fv(glGetUniformLocation(ProgramID, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void initRenderData()
{
    GLuint VBO;
    GLfloat vertecies[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 0.0f,

        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f
    };
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertecies), vertecies, GL_STATIC_DRAW);
    glBindVertexArray(VAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), (GLvoid*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

}

void compileShaders(char* vertexSource, char* fragmentSource)
{
     GLuint sVertex, sFragment;
    
    // Vertex Shader
    sVertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(sVertex, 1, &vertexSource, NULL);
    glCompileShader(sVertex);
    
    // Fragment Shader
    sFragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(sFragment, 1, &fragmentSource, NULL);
    glCompileShader(sFragment);
    
    // Shader Program
    ProgramID = glCreateProgram();
    glAttachShader(ProgramID, sVertex);
    glAttachShader(ProgramID, sFragment);
    glLinkProgram(ProgramID);
    
    // Delete the shaders as they're linked into our program now and no longer necessery
    glDeleteShader(sVertex);
    glDeleteShader(sFragment);
}
