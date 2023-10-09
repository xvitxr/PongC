#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define PONGC_PASSERT_ED(X, condition) printf("%s: %s\n", X, condition ? "enabled" : "disabled")
#define PONGC_FONT_FILE "/usr/share/fonts/TTF/Hack-Regular.ttf"

/* Globals */

const char* PongC_Help =
	"PongC initialization flags: \n\n"

	"--no-text           No text rendering.\n"
	"--resizable         Makes the game window resizable.\n"
	"--no-vsync          Disables VSync.\n"
	"--render-collisions Enables the redering of the collision boxes.\n"
	"--no-player1        Makes player 1 a computer.\n"
	"--no-player2        Makes player 2 a computer.\n"
  "--mouse-player1     Makes player 1 move with the mouse\n"
  "--mouse-player2     Makes player 2 move with the mouse\n\n"

	"Controls:\n\n"

	"F1   Shows this message\n"
	"F2   Decreases ball speed multiplier\n"
	"F3   Increases ball speed multiplier\n"
	"W    Moves player 1 up. (If enabled)\n"
	"S    Moves player 1 up. (If enabled)\n"
	"Up   Moves player 2 up. (If enabled)\n"
	"Down Moves player 2 up. (If enabled)\n";

SDL_Renderer* PongC_Renderer;
SDL_Window*   PongC_Window;

SDL_Event* PongC_Events;

TTF_Font* PongC_Font;

SDL_Color PongC_BGColor;

bool PongC_IsRunning = false;
bool PongC_Started   = false;

typedef struct
{
	int x;
	int y;

	int DirectionX;
	int DirectionY;

	uint32_t Radius;
} PongC_Ball_t;

PongC_Ball_t* PongC_Ball;
SDL_FRect     PongC_BallCollision;

typedef enum PongC_PlayerTypeEnum
{
	  PONGC_PLAYER_HUMAN
	, PONGC_PLAYER_COMPUTER
} PongC_PlayerType;

typedef struct
{
	int x;
	int y;

	int Width;
	int Height;

	int Side;
	int Type;
	int Id;
} PongC_Player_t;

#define PONGC_PLAYER_WIDTH 15
#define PONGC_PLAYER_HEIGHT 150

PongC_Player_t* PongC_Player1;
SDL_FRect       PongC_Player1Collision;
uint16_t		PongC_Player1Points;

PongC_Player_t* PongC_Player2;
SDL_FRect       PongC_Player2Collision;
uint16_t		PongC_Player2Points;

uint8_t PongC_GlobalPlayersId = 0;

float PongC_BallSpeedMultiplier = 3.f;


/* Flags */

bool PongC_CollisionRendering	= false;
bool PongC_RenderText			= true;
bool PongC_Resizable			= false;
bool PongC_VSync				= true;
bool PongC_NoPlayer1			= false;
bool PongC_NoPlayer2			= false;
bool PongC_MouseMovementPlayer1 = false;
bool PongC_MouseMovementPlayer2 = false;

int PongC_GetWindowW()
{
	int w;
	SDL_GetWindowSize(PongC_Window, &w, NULL);
	return w;
}
int PongC_GetWindowH()
{
	int h;
	SDL_GetWindowSize(PongC_Window, NULL, &h);
	return h;
}

bool PongC_IsKeyPressed(SDL_Scancode key)
{
	return SDL_GetKeyboardState(NULL)[key];
}

SDL_Color PongC_GetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	SDL_Color c;
	c.r = r;
	c.g = g;
	c.b = b;
	c.a = a;
	return c;
}

void PongC_RenderFillCircle(int centerX, int centerY, int radius)
{
	for (int y = -radius; y <= radius; y++)
	{
		for (int x = -radius; x <= radius; x++)
		{
			if (x * x + y * y <= radius * radius)
			{
				SDL_RenderDrawPoint(PongC_Renderer, centerX + x, centerY + y);
			}
		}
	}
}

void PongC_AssignPlayerCollision(PongC_Player_t* player, SDL_FRect* collision)
{
	if (collision != NULL)
	{
		collision->x = player->x;
		collision->y = player->y;
		collision->w = player->Width;
		collision->h = player->Height;
	}
}

void PongC_AssignBallCollision(PongC_Ball_t* ball, SDL_FRect* collision)
{
	if (collision != NULL)
	{
		collision->x = ball->x - ball->Radius / 1.5f;
		collision->y = ball->y - ball->Radius / 1.5f;
		collision->w = ball->Radius * 1.5f;
		collision->h = ball->Radius * 1.5f;
	}
}

PongC_Player_t* PongC_CreatePlayer(int screenSide, PongC_PlayerType type, SDL_FRect* collision)
{
	PongC_Player_t* p = (PongC_Player_t*) malloc(sizeof(PongC_Player_t));
	p->Width		  = PONGC_PLAYER_WIDTH;
	p->Height		  = PONGC_PLAYER_HEIGHT;
	p->Type			  = type;
	p->Id			  = PongC_GlobalPlayersId++;
	p->Side           = screenSide;

	if (screenSide == -1)
		p->x = 10;
	else
		p->x = (PongC_GetWindowW() - p->Width) - 10;

	p->y = (PongC_GetWindowH() / 2) - (p->Height / 2);

	PongC_AssignPlayerCollision(p, collision);

	return p;
}

int PongC_RenderDrawText(SDL_Renderer* renderer
	, const char* text
	, const SDL_FPoint* position
	, uint32_t size
	, SDL_RendererFlip flip
	, const SDL_Color color
)
{
	SDL_Surface* sur;

	sur = TTF_RenderText_Blended(PongC_Font, text, color);
	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, sur);

	SDL_FRect rect;

	rect.x = position->x, rect.y = position->y;
	rect.w = sur->w, rect.h = sur->h;

	SDL_RenderCopyF(
		renderer,
		tex,
		NULL,
		&rect
	);
	SDL_FreeSurface(sur);
	SDL_DestroyTexture(tex);

	return 0;
}

void PongC_Reset()
{
	PongC_Ball = (PongC_Ball_t*)malloc(sizeof(PongC_Ball_t));
	PongC_Ball->DirectionX = 0;
	PongC_Ball->DirectionY = 0;
	PongC_Ball->x = PongC_GetWindowW() / 2;
	PongC_Ball->y = PongC_GetWindowH() / 2;
	PongC_Ball->Radius = 15;

    PongC_GetWindowH();

	PongC_BallCollision.x = PongC_Ball->x - PongC_Ball->Radius / 1.5f;
	PongC_BallCollision.y = PongC_Ball->y - PongC_Ball->Radius / 1.5f;
	PongC_BallCollision.w = PongC_Ball->Radius * 1.5f;
	PongC_BallCollision.h = PongC_Ball->Radius * 1.5f;

	int player1Type = PongC_NoPlayer1 ? PONGC_PLAYER_COMPUTER : PONGC_PLAYER_HUMAN;
	int player2Type = PongC_NoPlayer2 ? PONGC_PLAYER_COMPUTER : PONGC_PLAYER_HUMAN;

	PongC_Player1 = PongC_CreatePlayer(-1, player1Type, &PongC_Player1Collision);
	PongC_Player2 = PongC_CreatePlayer(1, player2Type, &PongC_Player2Collision);

	PongC_Started = false;
	PongC_BallSpeedMultiplier = 3.f;

	PongC_GlobalPlayersId = 0;
}

// For humans.
void PongC_PlayerUpdateHuman(PongC_Player_t* player)
{
	int mouseY;
	SDL_GetMouseState(NULL, &mouseY);
	if (PongC_MouseMovementPlayer1) {
		if (!player->Id)
			player->y = mouseY - player->Height / 2;
	}
	if (PongC_MouseMovementPlayer2) {
		if (player->Id != 0)
			player->y = mouseY - player->Height / 2;
	}

	bool moveUp   = false;
	bool moveDown = false;

	if (!player->Id)
	{
		if (PongC_IsKeyPressed(SDL_SCANCODE_W))
			moveUp = true;
		if (PongC_IsKeyPressed(SDL_SCANCODE_S))
			moveDown = true;
	}
	else
	{
		if (PongC_IsKeyPressed(SDL_SCANCODE_UP))
			moveUp = true;
		if (PongC_IsKeyPressed(SDL_SCANCODE_DOWN))
			moveDown = true;
	}

	if (moveUp && !(player->y - 5 < 0))
		player->y -= 5;
	if (moveDown && !((player->y + player->Height) + 5 > PongC_GetWindowH()))
		player->y += 5;
}

// For computers.
void PongC_PlayerUpdateComputer(PongC_Player_t* player)
{
	// Will only move if the ball is going towards it.
	if (player->Side == PongC_Ball->DirectionX)
	{
		player->y = (PongC_Ball->y) + 5 * PongC_Ball->DirectionY;
	}

	if ((player->y - 5 < 0))
		player->y = 6;
	if ((player->y + player->Height) + 5 > PongC_GetWindowH())
		player->y = PongC_GetWindowH() - player->Height - 6;
}

void PongC_PlayerUpdate(PongC_Player_t* player)
{
	if (player->Type == PONGC_PLAYER_HUMAN)
		PongC_PlayerUpdateHuman(player);

	if (player->Type == PONGC_PLAYER_COMPUTER)
		PongC_PlayerUpdateComputer(player);

	if (player->Side == -1)
	{
		player->x = 10;
	}

	if (player->Side == 1)
	{
		player->x = (PongC_GetWindowW() - player->Width) - 10;
	}
}

void PongC_BallUpdate()
{
	if (PongC_Ball->DirectionX != 0)
		PongC_Ball->x += PongC_Ball->DirectionX * PongC_BallSpeedMultiplier;

	if (PongC_Ball->DirectionY != 0)
		PongC_Ball->y += PongC_Ball->DirectionY * (PongC_BallSpeedMultiplier - 1);
}

void PongC_HandleCollisions()
{
	SDL_FRect ballFuture = PongC_BallCollision;
	ballFuture.x += 5 * PongC_Ball->DirectionX;

	bool collision = SDL_HasIntersectionF(&ballFuture, &PongC_Player1Collision) ||
					 SDL_HasIntersectionF(&ballFuture, &PongC_Player2Collision);

	bool edge = (PongC_Ball->y - 1) < 0 || (PongC_Ball->y + 1) > PongC_GetWindowH();

	if (edge)
		PongC_Ball->DirectionY *= -1;
	else if (collision)
	{
		PongC_Ball->DirectionX *= -1;
		PongC_BallSpeedMultiplier += 0.15f;
	}

	if (PongC_Ball->x < 0)
	{
		PongC_Player2Points++;
		PongC_Reset();
	}
	if (PongC_Ball->x > PongC_GetWindowW())
	{
		PongC_Player1Points++;
		PongC_Reset();
	}
}

void PongC_DrawCollisions()
{
	SDL_SetRenderDrawColor(PongC_Renderer, 255, 0, 0, 255);

	/* Ball */
	SDL_RenderDrawRectF(PongC_Renderer, &PongC_BallCollision);

	/* Players */
	SDL_RenderDrawRectF(PongC_Renderer, &PongC_Player1Collision);
	SDL_RenderDrawRectF(PongC_Renderer, &PongC_Player2Collision);
}

void PongC_BallRender()
{
	SDL_SetRenderDrawColor(PongC_Renderer, 255, 255, 255, 255);
	PongC_RenderFillCircle(PongC_Ball->x, PongC_Ball->y, PongC_Ball->Radius);
}

void PongC_PlayerRender(SDL_FRect* playerCollision)
{
	SDL_SetRenderDrawColor(PongC_Renderer, 255, 255, 255, 255);
	SDL_RenderFillRectF(PongC_Renderer, playerCollision);
}

void PongC_ClearScreen(SDL_Color* color)
{
	SDL_SetRenderDrawColor(PongC_Renderer, color->r, color->g, color->b, color->a);
	SDL_RenderClear(PongC_Renderer);
}

void PongC_OnKeyDown(uint16_t mod, SDL_Scancode scan, SDL_Keycode key)
{
	switch (key)
	{
		case SDLK_F1:
			printf(PongC_Help);
			break;
	}
}

void PongC_OnKeyUp(uint16_t mod, SDL_Scancode scan, SDL_Keycode key)
{

}

int PongC_HandleEvents()
{
	switch (PongC_Events->type)
	{
		case SDL_QUIT: PongC_IsRunning = false; return 2;

		case SDL_KEYDOWN:
			PongC_OnKeyDown(PongC_Events->key.keysym.mod, PongC_Events->key.keysym.scancode, PongC_Events->key.keysym.sym);
			break;
		case SDL_KEYUP:
			PongC_OnKeyUp(PongC_Events->key.keysym.mod, PongC_Events->key.keysym.scancode, PongC_Events->key.keysym.sym);
			break;
	}
}

void PongC_Update()
{
	PongC_HandleCollisions();

	PongC_BallUpdate();
	PongC_AssignBallCollision(PongC_Ball, &PongC_BallCollision);

	PongC_PlayerUpdate(PongC_Player1);
	PongC_PlayerUpdate(PongC_Player2);

	PongC_AssignPlayerCollision(PongC_Player1, &PongC_Player1Collision);
	PongC_AssignPlayerCollision(PongC_Player2, &PongC_Player2Collision);

	if (PongC_IsKeyPressed(SDL_SCANCODE_SPACE) && !PongC_Started)
	{
		PongC_Started = true;

		PongC_Ball->DirectionX = -1;
		PongC_Ball->DirectionY = -1;
	}

	if (PongC_IsKeyPressed(SDL_SCANCODE_ESCAPE))
	{
		PongC_IsRunning = false;
	}

	if (PongC_IsKeyPressed(SDL_SCANCODE_F2))
	{
		PongC_BallSpeedMultiplier -= 0.5f;
	}

	if (PongC_IsKeyPressed(SDL_SCANCODE_F3))
	{
		PongC_BallSpeedMultiplier += 0.5f;
	}
}

void PongC_RenderUI()
{
	static char pointsP1Buffer[100];
	static char pointsP2Buffer[100];

	sprintf(pointsP1Buffer, "Player 1 Score: %d", PongC_Player1Points);
	sprintf(pointsP2Buffer, "Player 2 Score: %d", PongC_Player2Points);

	static SDL_FPoint pointsP1Pos;
	pointsP1Pos.x = 20;
	pointsP1Pos.y = 10;

	PongC_RenderDrawText(PongC_Renderer, pointsP1Buffer, &pointsP1Pos, 25, SDL_FLIP_NONE, PongC_GetColor(255, 255, 255, 255));

	static SDL_FPoint pointsP2Pos;
	pointsP2Pos.x = (PongC_GetWindowW() - 20) - sizeof(pointsP2Buffer) * 2;
	pointsP2Pos.y = 10;

	PongC_RenderDrawText(PongC_Renderer, pointsP2Buffer, &pointsP2Pos, 25, SDL_FLIP_NONE, PongC_GetColor(255, 255, 255, 255));
}

void PongC_Render()
{
	PongC_BallRender();

	PongC_PlayerRender(&PongC_Player1Collision);
	PongC_PlayerRender(&PongC_Player2Collision);

	/* Text rendering */

	if (PongC_RenderText)
		PongC_RenderUI();


	if (PongC_CollisionRendering)
		PongC_DrawCollisions();
}

/*
	Starts the game loop.
*/
void PongC_Run()
{
	PongC_IsRunning = true;

	while (PongC_IsRunning)
	{
		PongC_Events->type = 0;
		while (SDL_PollEvent(PongC_Events))
			PongC_HandleEvents();

		/*
			Afters handling events, may be the case of
			PongC_IsRunning being set to false, but the
			while-loop will keep running until the end,
			executing code that uses, now, null pointers.

			This prevents it.
		*/
		if (!PongC_IsRunning)
			break;

		PongC_Update();

		/* Rendering pipeline */

		PongC_ClearScreen(&PongC_BGColor); // Begin rendering
		PongC_Render();					   // General rendering
		SDL_RenderPresent(PongC_Renderer); // End rendering

	}

}

void PongC_InitWindowIcon()
{
	SDL_Surface* icon_sur;
	SDL_Texture* icon = SDL_CreateTexture(PongC_Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 16, 16);


	SDL_SetRenderTarget(PongC_Renderer, icon);

	SDL_SetRenderDrawColor(PongC_Renderer, 0, 0, 0, 0);
	SDL_RenderClear(PongC_Renderer);

	SDL_SetRenderDrawColor(PongC_Renderer, 255, 255, 0, 255);
	PongC_RenderFillCircle(8, 8, 6);

	{
		int w, h;
		SDL_QueryTexture(icon, NULL, NULL, &w, &h);
	}

	icon_sur = SDL_CreateRGBSurfaceWithFormat(0, 16, 16, 32, SDL_PIXELFORMAT_RGBA8888);

	SDL_RenderReadPixels(PongC_Renderer, NULL, icon_sur->format->format, icon_sur->pixels, icon_sur->pitch);

	SDL_SetWindowIcon(PongC_Window, icon_sur);

	SDL_SetRenderTarget(PongC_Renderer, NULL);
	SDL_DestroyTexture(icon);
	SDL_FreeSurface(icon_sur);
}

/*
	Initializes PongC components.

	@returns  2 when --help is passed.
			  0 on success.
			 -2 on SDL failure.
		     -3 on TTF failure.
			 -4 on Window failure.
			 -5 on Renderer failure.
*/
int PongC_Init(int argc, char* argv[])
{
#ifdef PONGC_DEBUG
	printf("[DEBUG MODE]\n\n");
#endif

	/* Pre-Init */
	printf("Total arguments: %d\n", argc);
	if (argc < 2);
	else
	{
		for (int i = 1; i < argc; i++)
		{
			if (!strcmp(argv[i], "--help"))
			{
				printf(PongC_Help);
				return 2;
			}

			if (!strcmp(argv[i], "--no-text"))
				PongC_RenderText = false;
			if (!strcmp(argv[i], "--resizable"))
				PongC_Resizable = true;
			if (!strcmp(argv[i], "--no-vsync"))
				PongC_VSync = false;
			if (!strcmp(argv[i], "--render-collisions"))
				PongC_CollisionRendering = true;
			if (!strcmp(argv[i], "--no-player1"))
				PongC_NoPlayer1 = true;
			if (!strcmp(argv[i], "--no-player2"))
				PongC_NoPlayer2 = true;
			if (!strcmp(argv[i], "--mouse-player1"))
				PongC_MouseMovementPlayer1 = true;
			if (!strcmp(argv[i], "--mouse-player2"))
				PongC_MouseMovementPlayer2 = true;
		}
	}

	PONGC_PASSERT_ED("Text rendering", PongC_RenderText);
	PONGC_PASSERT_ED("Resizable window", PongC_Resizable);
	PONGC_PASSERT_ED("Collision rendering", PongC_CollisionRendering);
	PONGC_PASSERT_ED("VSync", PongC_VSync);
	PONGC_PASSERT_ED("Player 1", !PongC_NoPlayer1);
	printf("\tMovement type: %s\n", PongC_MouseMovementPlayer1 ? "mouse." : "keys.");
	PONGC_PASSERT_ED("Player 2", !PongC_NoPlayer2);
	printf("\tMovement type: %s\n", PongC_MouseMovementPlayer2 ? "mouse." : "keys.");

	/* SDL initialization */

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		printf("SDL failed to initialize: %s\n", SDL_GetError());
		return -2;
	}
	else
		printf("SDL initialized successfully!\n");
	if (TTF_Init() != 0)
	{
		printf("TTF failed to initialize: %s\n", TTF_GetError());
		return -3;
	}
	else
		printf("TTF initialized successfully!\n");

	/* Font */
	PongC_Font = TTF_OpenFont(PONGC_FONT_FILE, 20);
	if (!PongC_Font)
	{
		printf("Failed to load font... [fira.ttf]: %s", TTF_GetError());
		PongC_RenderText = false;
	}

	// Window
	{
		uint32_t resizable = PongC_Resizable ? SDL_WINDOW_RESIZABLE : 0;

		PongC_Window = SDL_CreateWindow(
			"PongC"
			, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED
			, 700, 450
			, SDL_WINDOW_SHOWN | resizable
		);

		if (!PongC_Window)
		{
			printf("Failed to initialize window: %s\n", SDL_GetError());
			return -4;
		}
	}

	// Renderer
	{
		uint32_t vsync = PongC_VSync ? SDL_RENDERER_PRESENTVSYNC : 0;

		PongC_Renderer = SDL_CreateRenderer(PongC_Window, -1, vsync);

		if (!PongC_Renderer)
		{
			printf("Failed to initialize renderer: %s\n", SDL_GetError());
			return -5;
		}

		SDL_SetRenderDrawBlendMode(PongC_Renderer, SDL_BLENDMODE_ADD);
	}

	PongC_InitWindowIcon();

	PongC_Events = (SDL_Event*) malloc(sizeof(SDL_Event));

	/* Black background color */
	PongC_BGColor.r = 0;
	PongC_BGColor.g = 0;
	PongC_BGColor.b = 0;
	PongC_BGColor.a = 0;

	/* Ball initialization */

	PongC_Reset();

	return 0;
}

void PongC_Quit()
{
	SDL_DestroyWindow(PongC_Window);
	SDL_DestroyRenderer(PongC_Renderer);

	free(PongC_Events);

	free(PongC_Player1);
	free(PongC_Player2);
	free(PongC_Ball);

	TTF_CloseFont(PongC_Font);

	SDL_Quit();
	TTF_Quit();

	PongC_IsRunning = false;
}

int main(int argc, char* argv[])
{
	if (PongC_Init(argc, argv) != 0) return -1;
	PongC_Run();
	PongC_Quit();

	return 0;
}
