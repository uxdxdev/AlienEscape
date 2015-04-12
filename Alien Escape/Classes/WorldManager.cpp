#include "WorldManager.h"
#include "MenuScene.h"
#include "Definitions.h"
#include "Enemy.h"
#include <iostream>

WorldManager* WorldManager::m_Instance = 0;

WorldManager* WorldManager::getInstance()
{	
	if (m_Instance == nullptr)
		m_Instance = new WorldManager();
	return m_Instance;
}

WorldManager::~WorldManager()
{
	this->cleanUp();
}

WorldManager::WorldManager()
{	
	if (!init())
	{
		printf("Failed to initialize Game Scene!\n");
	}
}

bool WorldManager::init()
{	
	bool success = true;

	// Game DAO
	if (m_pGameDAO == nullptr)
	{
		m_pGameDAO = new GameDAO();
	}
	
	if (!isXMLFileExist())
	{
		m_pGameDAO->create();
		loadDefaultConfig(m_pGameDAO);
	}

	/*
	std::shared_ptr<std::vector<StoryPoint>> resources = m_pGameDAO->read();		

	for (int i = 0; i < resources->size(); i++)
	{
		for (int j = 0; j < resources->at(i).getStoryChoices()->size(); j++)
		{
			std::cout << resources->at(i).getStoryChoices()->at(j).getName() << " -> " << resources->at(i).getStoryChoices()->at(j).getValue() << std::endl;
		}
	}
	*/
	
	//m_pGameDAO->update(resources);

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
		{
			printf("Warning: Linear texture filtering not enabled!");
		}
		m_Window = SDL_CreateWindow(this->readDAO("GameName").c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (m_Window == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			m_Renderer = SDL_CreateRenderer(m_Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (m_Renderer == NULL)
			{
				printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				// PNG images
				SDL_SetRenderDrawColor(m_Renderer, 0xFF, 0xFF, 0xFF, 0xFF);
				int imgFlags = IMG_INIT_PNG;
				if (!(IMG_Init(imgFlags) & imgFlags))
				{
					printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
					success = false;
				}

				// Truetype Fonts
				if (TTF_Init() == -1)
				{
					printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
					success = false;
				}
			}
		}
	}

	currentRunningScene = nullptr;
	m_eGravitationalPull = GRAVITY_DOWN;
	m_RendererFlip = SDL_FLIP_NONE;
	m_nGameWorldSpeed = WORLD_SPEED;
		
	return success;
}

std::string WorldManager::readDAO(std::string name)
{
	std::shared_ptr<std::vector<StoryPoint>> resources = m_pGameDAO->read();

	for (int i = 0; i < resources->size(); i++)
	{
		for (int j = 0; j < resources->at(i).getStoryChoices()->size(); j++)
		{
			if (resources->at(i).getStoryChoices()->at(j).getName() == name)
			{
				return resources->at(i).getStoryChoices()->at(j).getValue();
			}						
		}
	}
}

bool WorldManager::isXMLFileExist()
{	
	std::string filepath = "";
	filepath.append(XMLDOC);
		
	FILE *fp;
	fopen_s(&fp, filepath.c_str(), "r");
	bool bRet = false;

	// If the file can be opened, it exists
	if (fp)
	{
		bRet = true;
		fclose(fp);
	}
	return bRet;
}

void WorldManager::loadDefaultConfig(IGameDAO* dao)
{
	std::shared_ptr<std::vector<StoryPoint>> resources = dao->read();

	// Menu Scene Config
	StoryPoint gameConfig;
	gameConfig.setName("GameConfig");
	gameConfig.addFilePath("GameName", "Alien Escape");	
	gameConfig.addFilePath("GameFont", "Resources/Fonts/go3v2.ttf");
	resources->push_back(gameConfig);

	// Menu Scene Config
	StoryPoint menuSceneConfig;	
	menuSceneConfig.setName("MenuScene");	
	menuSceneConfig.addFilePath("MenuSceneStartLabel", "Start Game");
	menuSceneConfig.addFilePath("MenuSceneBackground", "Resources/Backgrounds/MenuBackground.png");	
	resources->push_back(menuSceneConfig);

	// Menu Scene Config
	StoryPoint gameSceneConfig;
	gameSceneConfig.setName("GameScene");
	gameSceneConfig.addFilePath("GameScenePlayer", "Resources/Sprites/Player.png");
	gameSceneConfig.addFilePath("GameSceneEnemyAlien", "Resources/Sprites/EnemyAlien.png");
	gameSceneConfig.addFilePath("GameSceneBackgroundA", "Resources/Backgrounds/BackgroundA.png");
	gameSceneConfig.addFilePath("GameSceneBackgroundB", "Resources/Backgrounds/BackgroundB.png");
	gameSceneConfig.addFilePath("GameSceneMidgroundA", "Resources/Backgrounds/MidgroundA.png");
	gameSceneConfig.addFilePath("GameSceneMidgroundB", "Resources/Backgrounds/MidgroundB.png");	
	resources->push_back(gameSceneConfig);

	// HUD Config
	StoryPoint HUDConfig;
	HUDConfig.setName("HUDConfig");
	HUDConfig.addFilePath("HUDConfigPause", "Resources/Buttons/Pause.png");
	resources->push_back(HUDConfig);

	dao->update(resources);
}

bool WorldManager::checkCollisions()
{
	bool collision = false;

	for (int i = 0; i < m_vpGameNodes.size(); i++)
	{
		Enemy* tempEnemy = (Enemy*)m_vpGameNodes.at(i);
		if (m_pPlayer->getSprite()->checkCollision(tempEnemy->getSprite()->getBoundBox()) && tempEnemy->getState() == Character::ALIVE)
		{		
			printf("Collision Detected!\n");
			tempEnemy->setState(Character::DEAD);

			m_pPlayer->applyDamage(1);
			if (m_pPlayer->getLives() < 0)
			{
				m_pPlayer->setState(Character::DEAD);
			}				
			
			if (m_pPlayer->getState() == Character::DEAD)
			{
				collision = true;
				m_vpGameNodes.clear();
				// Game over
				MenuScene* menuScene = new MenuScene();
				WorldManager::getInstance()->runWithScene(menuScene);
			}			
		}
	}

	return collision;
}

void WorldManager::resetGame()
{	
	m_eGravitationalPull = GRAVITY_DOWN;
	m_RendererFlip = SDL_FLIP_NONE;
	m_nGameWorldSpeed = WORLD_SPEED;
}

void WorldManager::flipGravity()
{
	if (m_eGravitationalPull == GRAVITY_DOWN)
	{
		m_eGravitationalPull = GRAVITY_UP;
		m_RendererFlip = SDL_FLIP_VERTICAL;
	}
	else if (m_eGravitationalPull == GRAVITY_UP)
	{
		m_eGravitationalPull = GRAVITY_DOWN;
		m_RendererFlip = SDL_FLIP_NONE;
	}	
}

void WorldManager::runWithScene(Scene* scene)
{
	printf("WorldManager: Changing running scene...\n");
	if (currentRunningScene != nullptr)
	{		
		delete currentRunningScene;
	}

	currentRunningScene = scene;
	currentRunningScene->run();		
}

void WorldManager::cleanUp()
{
	delete m_Instance;
	m_Instance = nullptr;

	SDL_DestroyRenderer(m_Renderer);
	SDL_DestroyWindow(m_Window);
	m_Renderer = NULL;
	m_Window = NULL;

	//Quit SDL subsystems
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
	
}

