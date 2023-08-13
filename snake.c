#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <ncurses.h>
#include <locale.h>

#define CHARACTER		"(<<(||||)>>)|||"
#define	CHAR_FRAME_NB	15

#define FPS				60
#define FRAME_DURATION	(1000 / FPS)
#define UPS				60

#define BUFFER_WIDTH	80
#define BUFFER_HEIGHT	24
#define BUFFER_SIZE		(BUFFER_WIDTH * BUFFER_HEIGHT)

#define CENTER_SCREEN_X	(BUFFER_WIDTH / 2)
#define CENTER_SCREEN_Y	(BUFFER_HEIGHT / 2)

#define DEFAULT_SPEED	4.0

int		score;
double	speed;

char	**activeBuffer;
char	**inactiveBuffer;
char	**backgroundBuffer;

double	frameTime;

struct	timeval		frameStart;
struct	timeval		frameEnd;

typedef struct	segment
{
	int		index;
	int		posX;
	int		posY;
	struct	segment	*prev;
	struct	segment	*next;
} Segment;

typedef struct	snake
{
	Segment	*head;
	Segment	*tail;
	int		length;
} Snake;

typedef struct	fruit
{
	int		posX;
	int		posY;
} Fruit;

Snake	*createSnake(int posX, int posY);
void	addSegment(Snake *snake, int posX, int posY);
void	moveSnake(Snake *snake, int dx, int dy, int *grow);
void	renderSnake(Snake *snake, int frame);
void	freeSnake(Snake *snake);
Fruit	*spawnFruit(int maxWidth, int maxHeight);
void	renderFruit(Fruit *fruit, int frame);
void	displayScreen(char **activeBuffer);
char	**fillBufferWithFile(char *filePathName);

int		main(int argc, char **argv)
{
	srand(time(0));

	setlocale(LC_ALL, "en_US.UTF-8");

	initscr();
	curs_set(0); //Disable cursor printing
	cbreak();
	noecho();
	keypad(stdscr, 1);
	nodelay(stdscr, 1);

	int		running;
	int		key;
	int		lastKey;
	int		tick;
	int		grow;
	int		i;
	int		dx;
	int		dy;
	char	**temp;
	Segment	*currentSegm;
	Snake	*snake;
	Fruit	*fruit;

	running = 1;
	key = 0;
	tick = 0;
	grow = 2;
	speed = DEFAULT_SPEED;
	i = 0;
	score = 0;
	dx = 0;
	dy = 0;
	frameTime = 0;

	currentSegm = NULL;

	snake = createSnake(CENTER_SCREEN_X, CENTER_SCREEN_Y);
	fruit = spawnFruit(BUFFER_WIDTH, BUFFER_HEIGHT);

	if (!(activeBuffer = malloc(sizeof(char*) * BUFFER_HEIGHT + 1)))
		return (1);
	if (!(inactiveBuffer = malloc(sizeof(char*) * BUFFER_HEIGHT + 1)))
		return (1);

	for (int i = 0; i < BUFFER_HEIGHT; ++i)
	{
		if (!(activeBuffer[i] = malloc(sizeof(char) * BUFFER_WIDTH + 1)))
			return (1);
		if (!(inactiveBuffer[i] = malloc(sizeof(char) * BUFFER_WIDTH + 1)))
			return (1);
	}

	backgroundBuffer = fillBufferWithFile("background_1");

	// Game Loop
	while (running)
	{
		// Keep count of the current Tick within a second
		tick = ((tick + 1) % UPS);

		gettimeofday(&frameStart, NULL);


		key = getch();

		//Game Logic
		/*********************************************************/
		if (key != ERR)
		{			
			if (key == KEY_END)
				break;
			if (key == 'm')
			{
				free(backgroundBuffer);
				if (!(backgroundBuffer = fillBufferWithFile("background_1")))
					return (1);
			}

		}

		if (key == KEY_UP && dy <= 0)
		{
			dx = 0;
			dy = -1;
		}
		if (key == KEY_DOWN && dy >= 0)
		{
			dx = 0;
			dy = 1;
		}
		if (key == KEY_LEFT && dx <= 0)
		{
			dy = 0;
			dx = -2;
		}
		if (key == KEY_RIGHT && dx >= 0)
		{
			dy = 0;
			dx = 2;
		}
		/*********************************************************/
		

		if (key == 'c')
		{
			if (speed < UPS)
				speed += 0.3;		
			grow = 1;
		}
		if (key == 'f')
		{
			free(fruit);
			fruit = spawnFruit(BUFFER_WIDTH, BUFFER_HEIGHT);
		}

		//In case the snake catches the fruit
		if (snake->head->posX == fruit->posX && snake->head->posY == fruit->posY)
		{
			free(fruit);
			fruit = spawnFruit(BUFFER_WIDTH, BUFFER_HEIGHT);
			score++;
			grow++;
			if (speed < UPS)
				speed += 0.3;
		}

		//In case the snake touches himself
		currentSegm = snake->head;
		while (i < snake->length)
		{
			if (snake->head->posX == currentSegm->posX && snake->head->posY == currentSegm->posY)
				//dont know yet
				i;
			currentSegm = currentSegm->next;
			i++;
		}
		i = 0;

		if (tick % (UPS / (int)speed) == 0)
			moveSnake(snake, dx, dy, &grow);


		//Render Everything in the buffer

		//Render Background
		for (int i = 0; i < BUFFER_HEIGHT; ++i)
		{
			for (int j = 0; j < BUFFER_WIDTH; ++j)
			{
				//inactiveBuffer[i][j] = '0';
				if (i == 0 || i == BUFFER_HEIGHT - 1 || j == 0 || j == BUFFER_WIDTH - 1)
					inactiveBuffer[i][j] = '#';
				else
					inactiveBuffer[i][j] = ' ';
			}
		}
		
		renderSnake(snake, tick);
		renderFruit(fruit, tick);

		//Display Buffer to the screen
		displayScreen(activeBuffer);

		//Buffer Swap
		temp = activeBuffer;
		activeBuffer = inactiveBuffer;
		inactiveBuffer = temp;

		gettimeofday(&frameEnd, NULL);
		frameTime = (frameEnd.tv_sec - frameStart.tv_sec) * 1000 + (frameEnd.tv_usec - frameStart.tv_usec) / 1000;;
		//printf("\nFrameTime : %.0f\n", frameTime);
		if (frameTime < FRAME_DURATION)
			napms(FRAME_DURATION - frameTime);
	}

	freeSnake(snake);
	free(activeBuffer);
	free(inactiveBuffer);
	free(backgroundBuffer);

	endwin();

	return (0);
}

Snake	*createSnake(int posX, int posY)
{
	Snake	*snake;

	if (!(snake = malloc(sizeof(Snake))))
		return (NULL);
	if (!(snake->head = malloc(sizeof(Segment))))
	{
		free(snake);
		return (NULL);
	}
	
	snake->tail = snake->head;

	snake->head->index = 0;
	snake->head->posX = posX;
	snake->head->posY = posY;

	snake->head->prev = NULL;
	snake->tail->next = NULL;

	snake->length = 1;

	return (snake);
}

void	addSegment(Snake *snake, int posX, int posY)
{
	Segment	*newSegm;

	if (!(newSegm = malloc(sizeof(Segment))))
		return ;
	newSegm->index = snake->tail->index + 1;
	newSegm->posX = posX;
	newSegm->posY = posY;
	newSegm->prev = snake->tail;
	newSegm->next = NULL;
	snake->tail->next = newSegm;
	snake->tail = newSegm;
	snake->length++;
}

void	moveSnake(Snake *snake, int dx, int dy, int *grow)
{
	int	i;
	Segment	*current;

	i = 0;
	current = snake->tail;
	if ((*grow))
	{
		addSegment(snake, current->posX, current->posY);
		(*grow)--;
	}
	while (current)
	{
		if (current->prev)
		{
			current->posX = current->prev->posX;
			current->posY = current->prev->posY;
		}
		else
		{
			current->posX += dx;
			current->posY += dy;
		}
		if (current->posX > BUFFER_WIDTH - 2)
			current->posX = 2;
		if (current->posX < 2)
			current->posX = BUFFER_WIDTH - 2;
		if (current->posY > BUFFER_HEIGHT - 2)
			current->posY = 1;
		if (current->posY < 1)
			current->posY = BUFFER_HEIGHT - 2;
		current = current->prev;
		i++;
	}
}

void	renderSnake(Snake *snake, int frame)
{
	int	i;
	Segment	*current;
	//char	test[50] = "SnakeGameSoMuchFun!!!!!!!!";

	i = 0;
	current = snake->head;
	while (i < snake->length)
	{
		if (current == snake->head)
			inactiveBuffer[current->posY][current->posX] = 'O';
		else if (current == snake->tail)
			inactiveBuffer[current->posY][current->posX] = ':';
		else
			inactiveBuffer[current->posY][current->posX] = 'o';
		//'0' + snake->length - current->index
		current = current->next;
		i++;
	}
}

void	freeSnake(Snake *snake)
{
	Segment	*current;
	Segment	*temp;

	current = snake->head;
	while (current)
	{
		temp = current;
		current = current->next;
		free(temp);
	}
	free(snake);
}

Fruit	*spawnFruit(int maxWidth, int maxHeight)
{
	Fruit	*fruit;

	if (!(fruit = malloc(sizeof(Fruit))))
		return (NULL);
	fruit->posX = (rand() % (maxWidth - 2) / 2 + 1) * 2;
	fruit->posY = rand() % (maxHeight - 2) + 1;

	return (fruit);
}

void	renderFruit(Fruit *fruit, int frame)
{
	char	test[50] = "<<<^^^>>>vvv";
	inactiveBuffer[fruit->posY][fruit->posX] = test[frame % 12];
}

void	displayScreen(char **activeBuffer)
{
	int	i;

	for (i = 0; i < BUFFER_HEIGHT; i++)
		mvprintw(i, 0, activeBuffer[i]);
	mvprintw(i + 1, 2, "Score : %d\n", score);
	mvprintw(i + 1, BUFFER_WIDTH / 2 - 8, "Speed : %d\n", (int)speed);

	refresh();
}

char	**fillBufferWithFile(char *filePathName)
{
	char	**buffer;
	char	temp;
	FILE	*file;
	int		i;
	int		x;
	int		y;
	int		size;

	i = 0;
	x = 0;
	y = 0;
	if (!(file = fopen(filePathName, "r")))
		return (NULL);

	if (!(buffer = malloc(sizeof(char*) * BUFFER_HEIGHT + 1)))
		return (NULL);

	for (int i = 0; i < BUFFER_HEIGHT; ++i)
		if (!(buffer[i] = malloc(sizeof(char) * BUFFER_WIDTH + 1)))
			return (NULL);

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	rewind(file);

	while (x < BUFFER_HEIGHT)
	{
		while (y < BUFFER_WIDTH)
		{
			buffer[x][y] = 0;
			y++;
		}
		y = 0;
		x++;
	}

	x = 0;
	y = 0;

	while (x < BUFFER_HEIGHT)
	{
		while (y < BUFFER_WIDTH)
		{
			fread(&buffer[x][y], 1, 1, file);
			fseek(file, 0, SEEK_CUR);
			y++;
		}
		y = 0;
		x++;
		fseek(file, 1, SEEK_CUR);
	}

	fclose(file);
	return (buffer);

}