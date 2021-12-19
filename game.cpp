#include <iostream>
#include <Windows.h>
#include <string>
#include <algorithm>
using namespace std;

#include "Base.h"
#include "physics.h"

// Main 
class Worms : public Base 
{
public:

	Worms()
	{
		m_sAppName = L"Worms";
	}

private:
	Sprite* sprFont = nullptr;
	AudioSample* BgMusic = nullptr;
	// Map size
	int nMapWidth = 1280;
	int nMapHeight = 720;
	char* map = nullptr;
	int player = 0;

	// Camera Position
	float fCameraPosX = 0.0f;
	float fCameraPosY = 0.0f;
	float fCameraPosXTarget = 0.0f;
	float fCameraPosYTarget = 0.0f;

	
	list<unique_ptr<cPhysicsObject>> listObjects;

	cPhysicsObject* pObjectUnderControl = nullptr;		
	cPhysicsObject* pCameraTrackingObject = nullptr;	

	bool hitbox = false;
	bool bZoomOut = false;					// Render map
	bool bGameIsStable = false;				
	bool bEnablePlayerControl = true;		// The player keyboard enabled
	bool bEnableComputerControl = false;	// The player keyboard disabeled
	bool bEnergising = false;				// Weapon is charging
	bool bFireWeapon = false;				// Weapon fire
	bool bShowCountDown = false;			// Display time left
	bool bPlayerHasFired = false;			// Weapon has been fired
	

	float fEnergyLevel = 0.0f;				// Energy while charge
	float fTurnTime = 0.0f;					// Time left 

	
	vector<cTeam> vecTeams;

	
	int nCurrentTeam = 0;

	// AI control
	bool bAI_Jump = false;				
	bool bAI_AimLeft = false;			
	bool bAI_AimRight = false;			
	bool bAI_Energise = false;			


	float fAITargetAngle = 0.0f;		
	float fAITargetEnergy = 0.0f;		
	float fAISafePosition = 0.0f;		
	cWorm* pAITargetWorm = nullptr;		
	float fAITargetX = 0.0f;			
	float fAITargetY = 0.0f;

	bool end = false;

	enum GAME_STATE
	{
		GS_RESET = 0,
		GS_GENERATE_TERRAIN = 1,
		GS_GENERATING_TERRAIN,
		GS_ALLOCATE_UNITS,
		GS_ALLOCATING_UNITS,
		GS_START_PLAY,
		GS_CAMERA_MODE,
		GS_GAME_OVER1,
		GS_GAME_OVER2
	} nGameState, nNextState;


	enum AI_STATE
	{
		AI_ASSESS_ENVIRONMENT = 0,
		AI_MOVE,
		AI_CHOOSE_TARGET,
		AI_POSITION_FOR_TARGET,
		AI_AIM,
		AI_FIRE,
	} nAIState, nAINextState;

	void DrawBigText(int x, int y, std::string sText)
	{
		int i = 0;
		for (auto c : sText)
		{
			int sx = ((c - 32) % 16) * 8;
			int sy = ((c - 32) / 16) * 8;
			DrawPartialSprite(x + i * 8, y, sprFont, sx, sy, 8, 8);
			i++;
		}
	}

	virtual bool OnUserCreate()
	{
		BgMusic = new AudioSample(L"Assets/BgMusic.wav");
		sprFont = new Sprite(L"Assets/Text.spr");
		bool played = PlaySound(TEXT("Assets/BgMusic.wav"), NULL, SND_ASYNC);
	
		// Create Map
		map = new  char[nMapWidth * nMapHeight];
		memset(map, 0, nMapWidth * nMapHeight * sizeof(char));

		nGameState = GS_RESET;
		nNextState = GS_RESET;
		nAIState = AI_ASSESS_ENVIRONMENT;
		nAINextState = AI_ASSESS_ENVIRONMENT;

		bGameIsStable = false;
		return true;
	}

	virtual bool OnUserUpdate(float fElapsedTime)
	{
		if (m_keys[VK_TAB].bReleased)
			bZoomOut = !bZoomOut;

		float fMapScrollSpeed = 400.0f;
		if (m_keys[VK_F1].bPressed)
		{
			if (hitbox == false)
			{
				hitbox = true;
			}
			else
			{
				hitbox = false;
			}
			
		}
		/*if (m_mousePosX < 5) fCameraPosX -= fMapScrollSpeed * fElapsedTime;
		if (m_mousePosX > ScreenWidth() - 5) fCameraPosX += fMapScrollSpeed * fElapsedTime;
		if (m_mousePosY < 5) fCameraPosY -= fMapScrollSpeed * fElapsedTime;
		if (m_mousePosY > ScreenHeight() - 5) fCameraPosY += fMapScrollSpeed * fElapsedTime;*/

		switch (nGameState)
		{
		case GS_RESET:
		{
			bEnablePlayerControl = false;
			bGameIsStable = false;
			bPlayerHasFired = false;
			bShowCountDown = false;
			nNextState = GS_GENERATE_TERRAIN;
		}
		break;

		case GS_GENERATE_TERRAIN:
		{
			bZoomOut = true;
			CreateMap();
			bGameIsStable = false;
			bShowCountDown = false;
			nNextState = GS_GENERATING_TERRAIN;
		}
		break;

		case GS_GENERATING_TERRAIN:
		{
			bShowCountDown = false;
			if (bGameIsStable)
				nNextState = GS_ALLOCATE_UNITS;
		}
		break;

		case GS_ALLOCATE_UNITS:
		{
			// Deploy teams
			int nTeams = 4;
			int nWormsPerTeam = 2; //Amount of worms per team

			float fSpacePerTeam = (float)nMapWidth / (float)nTeams;
			float fSpacePerWorm = fSpacePerTeam / (nWormsPerTeam * 2.0f);

			// Create teams
			for (int t = 0; t < nTeams; t++)
			{
				vecTeams.emplace_back(cTeam());
				float fTeamMiddle = (fSpacePerTeam / 2.0f) + (t * fSpacePerTeam);
				for (int w = 0; w < nWormsPerTeam; w++)
				{
					float fWormX = fTeamMiddle - ((fSpacePerWorm * (float)nWormsPerTeam) / 2.0f) + w * fSpacePerWorm;
					float fWormY = 0.0f;

					// Add worms 
					cWorm* worm = new cWorm(fWormX, fWormY);
					worm->nTeam = t;
					listObjects.push_back(unique_ptr<cWorm>(worm));
					vecTeams[t].vecMembers.push_back(worm);
					vecTeams[t].nTeamSize = nWormsPerTeam;
				}

				vecTeams[t].nCurrentMember = 0;
			}

			pObjectUnderControl = vecTeams[0].vecMembers[vecTeams[0].nCurrentMember];
			pCameraTrackingObject = pObjectUnderControl;
			bShowCountDown = false;
			nNextState = GS_ALLOCATING_UNITS;
		}
		break;

		case GS_ALLOCATING_UNITS: 
		{
			if (bGameIsStable)
			{
				bEnablePlayerControl = true;
				bEnableComputerControl = false;
				fTurnTime = 15.0f;
				bZoomOut = false;
				nNextState = GS_START_PLAY;
			}
		}
		break;

		case GS_START_PLAY:
		{
			
			if (m_keys[VK_F2].bPressed)
			{
				if (player > 3)
				{
					player = 0;
				}
				else
				{
					player++;
				}
			}
			if (m_keys[VK_F3].bPressed)
			{
				nNextState = GS_GAME_OVER1;
			}
			bShowCountDown = true;

			if (bPlayerHasFired || fTurnTime <= 0.0f)
				nNextState = GS_CAMERA_MODE;
		}
		break;

		case GS_CAMERA_MODE: 
		{
			bEnableComputerControl = false;
			bEnablePlayerControl = false;
			bPlayerHasFired = false;
			bShowCountDown = false;
			fEnergyLevel = 0.0f;
			
			if (bGameIsStable)
			{
				
				int nOldTeam = nCurrentTeam;
				do {
					nCurrentTeam++;
					nCurrentTeam %= vecTeams.size();
				} while (!vecTeams[nCurrentTeam].IsTeamAlive());

				if (nCurrentTeam == 0) 
				{
					bEnablePlayerControl = true;	
					bEnableComputerControl = false;
				}
				else if (nCurrentTeam == 1 && player >= 1)
				{
					bEnablePlayerControl = true;
					bEnableComputerControl = false;
				}
				else if (nCurrentTeam == 2 && player >= 2)
				{
					bEnablePlayerControl = true;
					bEnableComputerControl = false;
				}
				else if (nCurrentTeam == 3 && player >= 3)
				{
					bEnablePlayerControl = true;
					bEnableComputerControl = false;
				}
				else // AI Team
				{
					bEnablePlayerControl = false;
					bEnableComputerControl = true;
				}

				pObjectUnderControl = vecTeams[nCurrentTeam].GetNextMember();
				pCameraTrackingObject = pObjectUnderControl;
				fTurnTime = 15.0f;
				bZoomOut = false;
				nNextState = GS_START_PLAY;

				if (nCurrentTeam == nOldTeam)
				{
					nNextState = GS_GAME_OVER1;
				}
			}
		}
		break;

		case GS_GAME_OVER1: 
		{
				end = true;
				bEnableComputerControl = false;
				bEnablePlayerControl = false;
				bZoomOut = true;
				bShowCountDown = false;

				

				nNextState = GS_GAME_OVER2;
		}
		break;

		case GS_GAME_OVER2: 
		{
			if (m_keys[L'R'].bPressed)
			{
				player = 0;
				end = false;
				listObjects.clear();
				vecTeams.clear();
				nNextState = GS_RESET;
			}
		}
		break;

		}

		// AI State Machine
		if (bEnableComputerControl)
		{
			switch (nAIState)
			{
			case AI_ASSESS_ENVIRONMENT:
			{

				int nAction = rand() % 3;
				if (nAction == 0) 
				{
					float fNearestAllyDistance = INFINITY; float fDirection = 0;
					cWorm* origin = (cWorm*)pObjectUnderControl;

					for (auto w : vecTeams[nCurrentTeam].vecMembers)
					{
						if (w != pObjectUnderControl)
						{
							if (fabs(w->px - origin->px) < fNearestAllyDistance)
							{
								fNearestAllyDistance = fabs(w->px - origin->px);
								fDirection = (w->px - origin->px) < 0.0f ? 1.0f : -1.0f;
							}
						}
					}

					if (fNearestAllyDistance < 50.0f)
						fAISafePosition = origin->px + fDirection * 80.0f;
					else
						fAISafePosition = origin->px;
				}

				if (nAction == 1) 
				{
					cWorm* origin = (cWorm*)pObjectUnderControl;
					float fDirection = ((float)(nMapWidth / 2.0f) - origin->px) < 0.0f ? -1.0f : 1.0f;
					fAISafePosition = origin->px + fDirection * 200.0f;
				}

				if (nAction == 2) 
				{
					cWorm* origin = (cWorm*)pObjectUnderControl;
					fAISafePosition = origin->px;
				}

				if (fAISafePosition <= 20.0f) fAISafePosition = 20.0f;
				if (fAISafePosition >= nMapWidth - 20.0f) fAISafePosition = nMapWidth - 20.0f;
				nAINextState = AI_MOVE;
			}
			break;

			case AI_MOVE:
			{
				cWorm* origin = (cWorm*)pObjectUnderControl;
				if (fTurnTime >= 8.0f && origin->px != fAISafePosition)
				{				
					if (fAISafePosition < origin->px && bGameIsStable)
					{
						origin->fShootAngle = -3.14159f * 0.6f;
						bAI_Jump = true;
						nAINextState = AI_MOVE;
					}

					if (fAISafePosition > origin->px && bGameIsStable)
					{
						origin->fShootAngle = -3.14159f * 0.4f;
						bAI_Jump = true;
						nAINextState = AI_MOVE;
					}
				}
				else
					nAINextState = AI_CHOOSE_TARGET;
			}
			break;

			case AI_CHOOSE_TARGET: 
			{
				bAI_Jump = false;

				cWorm* origin = (cWorm*)pObjectUnderControl;
				int nCurrentTeam = origin->nTeam;
				int nTargetTeam = 0;
				do {
					nTargetTeam = rand() % vecTeams.size();
				} while (nTargetTeam == nCurrentTeam || !vecTeams[nTargetTeam].IsTeamAlive());

				cWorm* mostHealthyWorm = vecTeams[nTargetTeam].vecMembers[0];
				for (auto w : vecTeams[nTargetTeam].vecMembers)
					if (w->fHealth > mostHealthyWorm->fHealth)
						mostHealthyWorm = w;

				pAITargetWorm = mostHealthyWorm;
				fAITargetX = mostHealthyWorm->px;
				fAITargetY = mostHealthyWorm->py;
				nAINextState = AI_POSITION_FOR_TARGET;
			}
			break;

			case AI_POSITION_FOR_TARGET: 
			{
				cWorm* origin = (cWorm*)pObjectUnderControl;
				float dy = -(fAITargetY - origin->py);
				float dx = -(fAITargetX - origin->px);
				float fSpeed = 30.0f;
				float fGravity = 2.0f;

				bAI_Jump = false;

				float a = fSpeed * fSpeed * fSpeed * fSpeed - fGravity * (fGravity * dx * dx + 2.0f * dy * fSpeed * fSpeed);

				if (a < 0) 
				{
					if (fTurnTime >= 5.0f)
					{
						if (pAITargetWorm->px < origin->px && bGameIsStable)
						{
							origin->fShootAngle = -3.14159f * 0.6f;
							bAI_Jump = true;
							nAINextState = AI_POSITION_FOR_TARGET;
						}

						if (pAITargetWorm->px > origin->px && bGameIsStable)
						{
							origin->fShootAngle = -3.14159f * 0.4f;
							bAI_Jump = true;
							nAINextState = AI_POSITION_FOR_TARGET;
						}
					}
					else
					{
						fAITargetAngle = origin->fShootAngle;
						fAITargetEnergy = 0.75f;
						nAINextState = AI_AIM;
					}
				}
				else
				{
					float b1 = fSpeed * fSpeed + sqrtf(a);
					float b2 = fSpeed * fSpeed - sqrtf(a);

					float fTheta1 = atanf(b1 / (fGravity * dx)); // Max Height
					float fTheta2 = atanf(b2 / (fGravity * dx)); // Min Height

					fAITargetAngle = fTheta1 - (dx > 0 ? 3.14159f : 0.0f);
					float fFireX = cosf(fAITargetAngle);
					float fFireY = sinf(fAITargetAngle);

					fAITargetEnergy = 0.75f;
					nAINextState = AI_AIM;
				}
			}
			break;

			case AI_AIM:
			{
				cWorm* worm = (cWorm*)pObjectUnderControl;

				bAI_AimLeft = false;
				bAI_AimRight = false;
				bAI_Jump = false;

				if (worm->fShootAngle < fAITargetAngle)
					bAI_AimRight = true;
				else
					bAI_AimLeft = true;

				if (fabs(worm->fShootAngle - fAITargetAngle) <= 0.001f)
				{
					bAI_AimLeft = false;
					bAI_AimRight = false;
					fEnergyLevel = 0.0f;
					nAINextState = AI_FIRE;
				}
				else
					nAINextState = AI_AIM;
			}
			break;

			case AI_FIRE:
			{
				bAI_Energise = true;
				bFireWeapon = false;
				bEnergising = true;

				if (fEnergyLevel >= fAITargetEnergy)
				{
					bFireWeapon = true;
					bAI_Energise = false;
					bEnergising = false;
					bEnableComputerControl = false;
					nAINextState = AI_ASSESS_ENVIRONMENT;
				}
			}
			break;

			}
		}

		// Turn Time + Players controls
		fTurnTime -= fElapsedTime;

		if (pObjectUnderControl != nullptr)
		{
			pObjectUnderControl->ax = 0.0f;

			if (pObjectUnderControl->bStable)
			{
				if ((bEnablePlayerControl && m_keys[L'W'].bPressed) || (bEnableComputerControl && bAI_Jump))
				{
					float a = ((cWorm*)pObjectUnderControl)->fShootAngle;

					pObjectUnderControl->vx = 4.0f * cosf(a);
					pObjectUnderControl->vy = 8.0f * sinf(a);
					pObjectUnderControl->bStable = false;

					bAI_Jump = false;
				}

				if ((bEnablePlayerControl && m_keys[L'D'].bHeld) || (bEnableComputerControl && bAI_AimRight))
				{
					cWorm* worm = (cWorm*)pObjectUnderControl;
					worm->fShootAngle += 1.0f * fElapsedTime;
					if (worm->fShootAngle > 3.14159f) worm->fShootAngle -= 3.14159f * 2.0f;
				}

				if ((bEnablePlayerControl && m_keys[L'A'].bHeld) || (bEnableComputerControl && bAI_AimLeft))
				{
					cWorm* worm = (cWorm*)pObjectUnderControl;
					worm->fShootAngle -= 1.0f * fElapsedTime;
					if (worm->fShootAngle < -3.14159f) worm->fShootAngle += 3.14159f * 2.0f;
				}

				if ((bEnablePlayerControl && m_keys[VK_SPACE].bPressed))
				{
					bFireWeapon = false;
					bEnergising = true;
					fEnergyLevel = 0.0f;
				}

				if ((bEnablePlayerControl && m_keys[VK_SPACE].bHeld) || (bEnableComputerControl && bAI_Energise))
				{
					
					if (bEnergising)
					{
						fEnergyLevel += 0.75f * fElapsedTime;
						if (fEnergyLevel >= 1.0f)
						{
							fEnergyLevel = 1.0f;
							bFireWeapon = true;
						}
					}
				}

				if ((bEnablePlayerControl && m_keys[VK_SPACE].bReleased))
				{
					if (bEnergising)
					{
						bFireWeapon = true;
					}

					bEnergising = false;
				}
			}

			if (pCameraTrackingObject != nullptr)
			{
				fCameraPosXTarget = pCameraTrackingObject->px - ScreenWidth() / 2;
				fCameraPosYTarget = pCameraTrackingObject->py - ScreenHeight() / 2;
				fCameraPosX += (fCameraPosXTarget - fCameraPosX) * 15.0f * fElapsedTime;
				fCameraPosY += (fCameraPosYTarget - fCameraPosY) * 15.0f * fElapsedTime;
			}

			if (bFireWeapon)
			{
				cWorm* worm = (cWorm*)pObjectUnderControl;

				float ox = worm->px;
				float oy = worm->py;

				float dx = cosf(worm->fShootAngle);
				float dy = sinf(worm->fShootAngle);

				cMissile* m = new cMissile(ox, oy, dx * 40.0f * fEnergyLevel, dy * 40.0f * fEnergyLevel);
				pCameraTrackingObject = m;
				listObjects.push_back(unique_ptr<cMissile>(m));

				bFireWeapon = false;
				fEnergyLevel = 0.0f;
				bEnergising = false;
				bPlayerHasFired = true;

				if (rand() % 100 >= 50)
					bZoomOut = true;
			}
		}

		if (fCameraPosX < 0) fCameraPosX = 0;
		if (fCameraPosX >= nMapWidth - ScreenWidth()) fCameraPosX = nMapWidth - ScreenWidth();
		if (fCameraPosY < 0) fCameraPosY = 0;
		if (fCameraPosY >= nMapHeight - ScreenHeight()) fCameraPosY = nMapHeight - ScreenHeight();

		for (int z = 0; z < 10; z++)
		{
			// Update physics
			for (auto& p : listObjects)
			{
				// Gravity
				p->ay += 2.0f;

				// Velocity
				p->vx += p->ax * fElapsedTime;
				p->vy += p->ay * fElapsedTime;

				// Position
				float fPotentialX = p->px + p->vx * fElapsedTime;
				float fPotentialY = p->py + p->vy * fElapsedTime;

				// Reset Acceleration
				p->ax = 0.0f;
				p->ay = 0.0f;

				p->bStable = false;

				// Collision
				float fAngle = atan2f(p->vy, p->vx);
				float fResponseX = 0;
				float fResponseY = 0;
				bool bCollision = false;
				for (float r = fAngle - 3.14159f / 2.0f; r < fAngle + 3.14159f / 2.0f; r += 3.14159f / 4.0f)
				{
					float fTestPosX = (p->radius) * cosf(r) + fPotentialX;
					float fTestPosY = (p->radius) * sinf(r) + fPotentialY;

					if (fTestPosX >= nMapWidth) fTestPosX = nMapWidth - 1;
					if (fTestPosY >= nMapHeight) fTestPosY = nMapHeight - 1;
					if (fTestPosX < 0) fTestPosX = 0;
					if (fTestPosY < 0) fTestPosY = 0;

					if (map[(int)fTestPosY * nMapWidth + (int)fTestPosX] > 0)
					{
						fResponseX += fPotentialX - fTestPosX;
						fResponseY += fPotentialY - fTestPosY;
						bCollision = true;
					}
				}

				float fMagVelocity = sqrtf(p->vx * p->vx + p->vy * p->vy);
				float fMagResponse = sqrtf(fResponseX * fResponseX + fResponseY * fResponseY);

				if (p->px < 0 || p->px > nMapWidth || p->py <0 || p->py > nMapHeight)
					p->bDead = true;

				if (bCollision)
				{
					p->bStable = true;

					float dot = p->vx * (fResponseX / fMagResponse) + p->vy * (fResponseY / fMagResponse);

					// Friction 
					p->vx = p->fFriction * (-2.0f * dot * (fResponseX / fMagResponse) + p->vx);
					p->vy = p->fFriction * (-2.0f * dot * (fResponseY / fMagResponse) + p->vy);

					if (p->nBounceBeforeDeath > 0)
					{
						p->nBounceBeforeDeath--;
						p->bDead = p->nBounceBeforeDeath == 0;
						if (p->bDead)
						{
							int nResponse = p->BounceDeathAction();
							if (nResponse > 0)
							{
								Boom(p->px, p->py, nResponse);
								pCameraTrackingObject = nullptr;
							}
						}
					}
				}
				else
				{
					p->px = fPotentialX;
					p->py = fPotentialY;
				}

				if (fMagVelocity < 0.1f) p->bStable = true;
			}

			listObjects.remove_if([](unique_ptr<cPhysicsObject>& o) {return o->bDead; });
		}

		// Draw 
		if (!bZoomOut & hitbox == true)
		{
			for (int x = 0; x < ScreenWidth(); x++)
				for (int y = 0; y < ScreenHeight(); y++)
				{
					switch (map[(y + (int)fCameraPosY) * nMapWidth + (x + (int)fCameraPosX)])
					{
					case -1:Draw(x, y, PIXEL_SOLID, FG_DARK_BLUE); break;
					case -2:Draw(x, y, PIXEL_QUARTER, FG_BLUE | BG_DARK_BLUE); break;
					case -3:Draw(x, y, PIXEL_HALF, FG_BLUE | BG_DARK_BLUE); break;
					case -4:Draw(x, y, PIXEL_THREEQUARTERS, FG_BLUE | BG_DARK_BLUE); break;
					case -5:Draw(x, y, PIXEL_SOLID, FG_BLUE); break;
					case -6:Draw(x, y, PIXEL_QUARTER, FG_CYAN | BG_BLUE); break;
					case -7:Draw(x, y, PIXEL_HALF, FG_CYAN | BG_BLUE); break;
					case -8:Draw(x, y, PIXEL_THREEQUARTERS, FG_CYAN | BG_BLUE); break;
					case 0:	Draw(x, y, PIXEL_SOLID, FG_CYAN); break;
					case 1:	Draw(x, y, PIXEL_SOLID, FG_RED);	break;
					}
				}

			// Draw 
			for (auto& p : listObjects)
			{
				p->Draw(this, fCameraPosX, fCameraPosY);

				cWorm* worm = (cWorm*)pObjectUnderControl;
				if (p.get() == worm)
				{
					float cx = worm->px + 8.0f * cosf(worm->fShootAngle) - fCameraPosX;
					float cy = worm->py + 8.0f * sinf(worm->fShootAngle) - fCameraPosY;

					Draw(cx, cy, PIXEL_SOLID, FG_BLACK);
					Draw(cx + 1, cy, PIXEL_SOLID, FG_BLACK);
					Draw(cx - 1, cy, PIXEL_SOLID, FG_BLACK);
					Draw(cx, cy + 1, PIXEL_SOLID, FG_BLACK);
					Draw(cx, cy - 1, PIXEL_SOLID, FG_BLACK);

					for (int i = 0; i < 11 * fEnergyLevel; i++)
					{
						Draw(worm->px - 5 + i - fCameraPosX, worm->py - 12 - fCameraPosY, PIXEL_SOLID, FG_GREEN);
						Draw(worm->px - 5 + i - fCameraPosX, worm->py - 11 - fCameraPosY, PIXEL_SOLID, FG_RED);
					}
				}
			}
			DrawBigText(80, 30, "Map Colliders");
		}
		else if (!bZoomOut)
		{
			for (int x = 0; x < ScreenWidth(); x++)
				for (int y = 0; y < ScreenHeight(); y++)
				{
					switch (map[(y + (int)fCameraPosY) * nMapWidth + (x + (int)fCameraPosX)])
					{
					case -1:Draw(x, y, PIXEL_SOLID, FG_DARK_BLUE); break;
					case -2:Draw(x, y, PIXEL_QUARTER, FG_BLUE | BG_DARK_BLUE); break;
					case -3:Draw(x, y, PIXEL_HALF, FG_BLUE | BG_DARK_BLUE); break;
					case -4:Draw(x, y, PIXEL_THREEQUARTERS, FG_BLUE | BG_DARK_BLUE); break;
					case -5:Draw(x, y, PIXEL_SOLID, FG_BLUE); break;
					case -6:Draw(x, y, PIXEL_QUARTER, FG_CYAN | BG_BLUE); break;
					case -7:Draw(x, y, PIXEL_HALF, FG_CYAN | BG_BLUE); break;
					case -8:Draw(x, y, PIXEL_THREEQUARTERS, FG_CYAN | BG_BLUE); break;
					case 0:	Draw(x, y, PIXEL_SOLID, FG_CYAN); break;
					case 1:	Draw(x, y, PIXEL_SOLID, FG_DARK_GREEN);	break;
					}
				}

			// Draw 
			for (auto& p : listObjects)
			{
				p->Draw(this, fCameraPosX, fCameraPosY);

				cWorm* worm = (cWorm*)pObjectUnderControl;
				if (p.get() == worm)
				{
					float cx = worm->px + 8.0f * cosf(worm->fShootAngle) - fCameraPosX;
					float cy = worm->py + 8.0f * sinf(worm->fShootAngle) - fCameraPosY;

					Draw(cx, cy, PIXEL_SOLID, FG_BLACK);
					Draw(cx + 1, cy, PIXEL_SOLID, FG_BLACK);
					Draw(cx - 1, cy, PIXEL_SOLID, FG_BLACK);
					Draw(cx, cy + 1, PIXEL_SOLID, FG_BLACK);
					Draw(cx, cy - 1, PIXEL_SOLID, FG_BLACK);

					for (int i = 0; i < 11 * fEnergyLevel; i++)
					{
						Draw(worm->px - 5 + i - fCameraPosX, worm->py - 12 - fCameraPosY, PIXEL_SOLID, FG_GREEN);
						Draw(worm->px - 5 + i - fCameraPosX, worm->py - 11 - fCameraPosY, PIXEL_SOLID, FG_RED);
					}
				}
			}
		}
		else if (hitbox == true)
		{
			for (int x = 0; x < ScreenWidth(); x++)
				for (int y = 0; y < ScreenHeight(); y++)
				{
					float fx = (float)x / (float)ScreenWidth() * (float)nMapWidth;
					float fy = (float)y / (float)ScreenHeight() * (float)nMapHeight;

					switch (map[((int)fy) * nMapWidth + ((int)fx)])
					{
					case -1:Draw(x, y, PIXEL_SOLID, FG_DARK_BLUE); break;
					case -2:Draw(x, y, PIXEL_QUARTER, FG_BLUE | BG_DARK_BLUE); break;
					case -3:Draw(x, y, PIXEL_HALF, FG_BLUE | BG_DARK_BLUE); break;
					case -4:Draw(x, y, PIXEL_THREEQUARTERS, FG_BLUE | BG_DARK_BLUE); break;
					case -5:Draw(x, y, PIXEL_SOLID, FG_BLUE); break;
					case -6:Draw(x, y, PIXEL_QUARTER, FG_CYAN | BG_BLUE); break;
					case -7:Draw(x, y, PIXEL_HALF, FG_CYAN | BG_BLUE); break;
					case -8:Draw(x, y, PIXEL_THREEQUARTERS, FG_CYAN | BG_BLUE); break;
					case 0:	Draw(x, y, PIXEL_SOLID, FG_CYAN); break;
					case 1:	Draw(x, y, PIXEL_SOLID, FG_RED);	break;
					}
				}

			for (auto& p : listObjects)
				p->Draw(this, p->px - (p->px / (float)nMapWidth) * (float)ScreenWidth(),
					p->py - (p->py / (float)nMapHeight) * (float)ScreenHeight(), true);
			DrawBigText(80, 30, "Map Colliders");
		}
		else
		{
		for (int x = 0; x < ScreenWidth(); x++)
			for (int y = 0; y < ScreenHeight(); y++)
			{
				float fx = (float)x / (float)ScreenWidth() * (float)nMapWidth;
				float fy = (float)y / (float)ScreenHeight() * (float)nMapHeight;

				switch (map[((int)fy) * nMapWidth + ((int)fx)])
				{
				case -1:Draw(x, y, PIXEL_SOLID, FG_DARK_BLUE); break;
				case -2:Draw(x, y, PIXEL_QUARTER, FG_BLUE | BG_DARK_BLUE); break;
				case -3:Draw(x, y, PIXEL_HALF, FG_BLUE | BG_DARK_BLUE); break;
				case -4:Draw(x, y, PIXEL_THREEQUARTERS, FG_BLUE | BG_DARK_BLUE); break;
				case -5:Draw(x, y, PIXEL_SOLID, FG_BLUE); break;
				case -6:Draw(x, y, PIXEL_QUARTER, FG_CYAN | BG_BLUE); break;
				case -7:Draw(x, y, PIXEL_HALF, FG_CYAN | BG_BLUE); break;
				case -8:Draw(x, y, PIXEL_THREEQUARTERS, FG_CYAN | BG_BLUE); break;
				case 0:	Draw(x, y, PIXEL_SOLID, FG_CYAN); break;
				case 1:	Draw(x, y, PIXEL_SOLID, FG_DARK_GREEN);	break;
				}
			}

		for (auto& p : listObjects)
			p->Draw(this, p->px - (p->px / (float)nMapWidth) * (float)ScreenWidth(),
				p->py - (p->py / (float)nMapHeight) * (float)ScreenHeight(), true);
		}
		bGameIsStable = true;
		for (auto& p : listObjects)
			if (!p->bStable)
			{
				bGameIsStable = false;
				break;
			}

		// Draw Team HP Bars
		for (size_t t = 0; t < vecTeams.size(); t++)
		{
			float fTotalHealth = 0.0f;
			float fMaxHealth = (float)vecTeams[t].nTeamSize;
			for (auto w : vecTeams[t].vecMembers) 
				fTotalHealth += w->fHealth;

			int cols[] = { FG_RED, FG_BLUE, FG_MAGENTA, FG_GREEN };
			Fill(4, 4 + t * 4, (fTotalHealth / fMaxHealth) * (float)(ScreenWidth() - 8) + 4, 4 + t * 4 + 3, PIXEL_SOLID, cols[t]);
		}

		if (bShowCountDown)
		{
			wchar_t d[] = L"w$]m.k{\%\x7Fo"; int tx = 4, ty = vecTeams.size() * 4 + 8;
			for (int r = 0; r < 13; r++) {
				for (int c = 0; c < ((fTurnTime < 10.0f) ? 1 : 2); c++) {
					int a = to_wstring(fTurnTime)[c] - 48; if (!(r % 6)) {
						DrawStringAlpha(tx,
							ty, wstring((d[a] & (1 << (r / 2)) ? L" #####  " : L"        ")), FG_BLACK);
						tx += 8;
					}
					else {
						DrawStringAlpha(tx, ty, wstring((d[a] & (1 << (r < 6 ? 1 : 4)) ?
							L"#     " : L"      ")), FG_BLACK); tx += 6; DrawStringAlpha(tx, ty, wstring
							((d[a] & (1 << (r < 6 ? 2 : 5)) ? L"# " : L"  ")), FG_BLACK); tx += 2;
					}
				}ty++; tx = 4;
			}
		}
		if (end == true)
		{
			if (nCurrentTeam == 0)
			{
				DrawBigText(80, 30, "RED Team Won");
				DrawBigText(50, 40, "Press R to play again");
				//DrawStringAlpha(118, 30, wstring(L"Team RED won"), FG_WHITE);
				//DrawStringAlpha(118, 32, wstring(L"Press_R_to_play_again"), FG_WHITE);
			}
			else if (nCurrentTeam == 1)
			{
				DrawBigText(80, 30, "BLUE Team Won");
				DrawBigText(50, 40, "Press R to play again");
				//DrawStringAlpha(118, 30, wstring(L"Team BLUE won"), FG_WHITE);
				//DrawStringAlpha(118, 32, wstring(L"Press_R_to_play_again"), FG_WHITE);
			}
			else if (nCurrentTeam == 2)
			{
				DrawBigText(80, 30, "PINK Team Won");
				DrawBigText(50, 40, "Press R to play again");
				//DrawStringAlpha(118, 30, wstring(L"Team PINK won"), FG_WHITE);
				//DrawStringAlpha(118, 32, wstring(L"Press_R_to_play_again"), FG_WHITE);
			}
			else if (nCurrentTeam == 3)
			{
				DrawBigText(80, 30, "GREEN Team Won");
				DrawBigText(50, 40, "Press R to play again");
				//DrawStringAlpha(118, 30, wstring(L"Team GREEN won"), FG_WHITE);
				//DrawStringAlpha(118, 32, wstring(L"Press_R_to_play_again"), FG_WHITE);
			}
		}

		nGameState = nNextState;
		nAIState = nAINextState;

		return true;
	}
	
	


	void Boom(float fWorldX, float fWorldY, float fRadius)
	{
		auto CircleBresenham = [&](int xc, int yc, int r)
		{
			int x = 0;
			int y = r;
			int p = 3 - 2 * r;
			if (!r) return;

			auto drawline = [&](int sx, int ex, int ny)
			{
				for (int i = sx; i < ex; i++)
					if (ny >= 0 && ny < nMapHeight && i >= 0 && i < nMapWidth)
						map[ny * nMapWidth + i] = 0;
			};

			while (y >= x) 
			{
				drawline(xc - x, xc + x, yc - y);
				drawline(xc - y, xc + y, yc - x);
				drawline(xc - x, xc + x, yc + y);
				drawline(xc - y, xc + y, yc + x);
				if (p < 0) p += 4 * x++ + 6;
				else p += 4 * (x++ - y--) + 10;
			}
		};

		int bx = (int)fWorldX;
		int by = (int)fWorldY;

		CircleBresenham(fWorldX, fWorldY, fRadius);

		for (auto& p : listObjects)
		{
			float dx = p->px - fWorldX;
			float dy = p->py - fWorldY;
			float fDist = sqrt(dx * dx + dy * dy);
			if (fDist < 0.0001f) fDist = 0.0001f;
			if (fDist < fRadius)
			{
				p->vx = (dx / fDist) * fRadius;
				p->vy = (dy / fDist) * fRadius;
				p->Damage(((fRadius - fDist) / fRadius) * 0.8f);
				p->bStable = false;
			}
		}

		for (int i = 0; i < (int)fRadius; i++)
			listObjects.push_back(unique_ptr<Rock>(new Rock(fWorldX, fWorldY)));
	}

	void PerlinNoise1D(int nCount, float* fSeed, int nOctaves, float fBias, float* fOutput)
	{
		// Perlin Noise
		for (int x = 0; x < nCount; x++)
		{
			float fNoise = 0.0f;
			float fScaleAcc = 0.0f;
			float fScale = 1.0f;

			for (int o = 0; o < nOctaves; o++)
			{
				int nPitch = nCount >> o;
				int nSample1 = (x / nPitch) * nPitch;
				int nSample2 = (nSample1 + nPitch) % nCount;
				float fBlend = (float)(x - nSample1) / (float)nPitch;
				float fSample = (1.0f - fBlend) * fSeed[nSample1] + fBlend * fSeed[nSample2];
				fScaleAcc += fScale;
				fNoise += fSample * fScale;
				fScale = fScale / fBias;
			}

			fOutput[x] = fNoise / fScaleAcc;
		}
	}

	void CreateMap()
	{
		// Perlin Noise
		float* fSurface = new float[nMapWidth];
		float* fNoiseSeed = new float[nMapWidth];
		for (int i = 0; i < nMapWidth; i++)
			fNoiseSeed[i] = (float)rand() / (float)RAND_MAX;

		fNoiseSeed[0] = 0.5f;
		PerlinNoise1D(nMapWidth, fNoiseSeed, 8, 2.0f, fSurface);

		for (int x = 0; x < nMapWidth; x++)
			for (int y = 0; y < nMapHeight; y++)
			{
				if (y >= fSurface[x] * nMapHeight)
					map[y * nMapWidth + x] = 1;
				else
				{
					if ((float)y < (float)nMapHeight / 3.0f)
						map[y * nMapWidth + x] = (-8.0f * ((float)y / (nMapHeight / 3.0f))) - 1.0f;
					else
						map[y * nMapWidth + x] = 0;
				}
			}

		delete[] fSurface;
		delete[] fNoiseSeed;
	}


};



int main()
{
	Worms game;
	game.ConstructConsole(256, 160, 6, 6);
	game.Start();
	return 0;
}

/*int main()
{
	Worms game;
	game.ConstructConsole(MAX_X / PIX_X, MAX_Y / PIX_Y, PIX_X, PIX_Y);
	game.Start();

	return 0;
}*/