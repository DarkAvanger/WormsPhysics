// Changing any physics value while playing will cause the game to break, any physics values should be changed before compiling


#include <iostream>
#include <string>
#include <algorithm>
using namespace std;

#include "Base.h"


class cPhysicsObject
{
public:
	cPhysicsObject(float x = 0.0f, float y = 0.0f)
	{
		px = x;
		py = y;
	}

public:
	float px = 0.0f;				// Position
	float py = 0.0f;
	float vx = 0.0f;				// Velocity
	float vy = 0.0f;
	float ax = 0.0f;				// Acceleration
	float ay = 0.0f;
	float radius = 4.0f;			// Rectangle for collisions
	float fFriction = 0.0f;		
	int nBounceBeforeDeath = -1;	// How many time object can bounce before death
	bool bDead;						
	bool bStable = false;			

	
	virtual void Draw(Base* engine, float fOffsetX, float fOffsetY, bool bPixel = false) = 0;
	virtual int BounceDeathAction() = 0;
	virtual bool Damage(float d) = 0;
};

class Rock : public cPhysicsObject 
{
public:
	Rock(float x = 0.0f, float y = 0.0f) : cPhysicsObject(x, y)
	{
		vx = 10.0f * cosf(((float)rand() / (float)RAND_MAX) * 2.0f * 3.14159f);
		vy = 10.0f * sinf(((float)rand() / (float)RAND_MAX) * 2.0f * 3.14159f);
		radius = 1.0f;
		fFriction = 0.8f;
		bDead = false;
		bStable = false;
		nBounceBeforeDeath = 2; 
	}

	virtual void Draw(Base* engine, float fOffsetX, float fOffsetY, bool bPixel = false)
	{
		engine->DrawWireFrameModel(vecModel, px - fOffsetX, py - fOffsetY, atan2f(vy, vx), bPixel ? 0.5f : radius, FG_DARK_GREEN); //Particle explosion
	}

	virtual int BounceDeathAction()
	{
		return 0; 
	}

	virtual bool Damage(float d)
	{
		return true; 
	}

private:
	static vector<pair<float, float>> vecModel;

};

vector<pair<float, float>>DefineRock()
{
	vector<pair<float, float>> vecModel;
	vecModel.push_back({ 0.0f, 0.0f });
	vecModel.push_back({ 1.0f, 0.0f });
	vecModel.push_back({ 1.0f, 1.0f });
	vecModel.push_back({ 0.0f, 1.0f });
	return vecModel;
}

vector<pair<float, float>> Rock::vecModel = DefineRock();

class cMissile : public cPhysicsObject 
{
public:
	cMissile(float x = 0.0f, float y = 0.0f, float _vx = 0.0f, float _vy = 0.0f) : cPhysicsObject(x, y)
	{
		radius = 2.5f;
		fFriction = 0.5f;
		vx = _vx;
		vy = _vy;
		bDead = false;
		nBounceBeforeDeath = 1;
		bStable = false;
	}

	virtual void Draw(Base* engine, float fOffsetX, float fOffsetY, bool bPixel = false)
	{
		engine->DrawWireFrameModel(vecModel, px - fOffsetX, py - fOffsetY, atan2f(vy, vx), bPixel ? 0.5f : radius, FG_BLACK);
	}

	virtual int BounceDeathAction()
	{
		return 20; 
	}

	virtual bool Damage(float d)
	{
		return true;
	}

private:
	static vector<pair<float, float>> vecModel;
};

vector<pair<float, float>> DefineMissile()
{
	vector<pair<float, float>> vecModel;
	vecModel.push_back({ 0.0f, 0.0f });
	vecModel.push_back({ 1.0f, 1.0f });
	vecModel.push_back({ 2.0f, 1.0f });
	vecModel.push_back({ 2.5f, 0.0f });
	vecModel.push_back({ 2.0f, -1.0f });
	vecModel.push_back({ 1.0f, -1.0f });
	vecModel.push_back({ 0.0f, 0.0f });
	vecModel.push_back({ -1.0f, -1.0f });
	vecModel.push_back({ -2.5f, -1.0f });
	vecModel.push_back({ -2.0f, 0.0f });
	vecModel.push_back({ -2.5f, 1.0f });
	vecModel.push_back({ -1.0f, 1.0f });

	for (auto& v : vecModel)
	{
		v.first /= 1.5f; v.second /= 1.5f;
	}
	return vecModel;
}

vector<pair<float, float>> cMissile::vecModel = DefineMissile();

class cWorm : public cPhysicsObject 
{
public:
	cWorm(float x = 0.0f, float y = 0.0f) : cPhysicsObject(x, y)
	{
		radius = 3.5f;
		fFriction = 0.2f;
		bDead = false;
		nBounceBeforeDeath = -1;
		bStable = false;

		if (sprWorm == nullptr)
			sprWorm = new Sprite(L"Assets/worms1.spr");
	}

	virtual void Draw(Base* engine, float fOffsetX, float fOffsetY, bool bPixel = false)
	{
		if (bIsPlayable) 
		{
			engine->DrawPartialSprite(px - fOffsetX - radius, py - fOffsetY - radius, sprWorm, nTeam * 8, 0, 8, 8);

			for (int i = 0; i < 11 * fHealth; i++)
			{
				engine->Draw(px - 5 + i - fOffsetX, py + 5 - fOffsetY, PIXEL_SOLID, FG_BLUE);
				engine->Draw(px - 5 + i - fOffsetX, py + 6 - fOffsetY, PIXEL_SOLID, FG_BLUE);
			}
		}
		else 
		{
			engine->DrawPartialSprite(px - fOffsetX - radius, py - fOffsetY - radius, sprWorm, nTeam * 8, 8, 8, 8);
		}
	}

	virtual int BounceDeathAction()
	{
		return 0; 
	}

	virtual bool Damage(float d) 
	{
		fHealth -= d;
		if (fHealth <= 0)
		{ 
			fHealth = 0.0f;
			bIsPlayable = false;
		}
		return fHealth > 0;
	}

public:
	float fShootAngle = 0.0f;
	float fHealth = 1.0f;
	int nTeam = 0;	
	bool bIsPlayable = true;

private:
	static Sprite* sprWorm;
};

Sprite* cWorm::sprWorm = nullptr;


class cTeam 
{
public:
	vector<cWorm*> vecMembers;
	int nCurrentMember = 0;		
	int nTeamSize = 0;			

	bool IsTeamAlive()
	{
		bool bAllDead = false;
		for (auto w : vecMembers)
			bAllDead |= (w->fHealth > 0.0f);
		return bAllDead;
	}

	cWorm* GetNextMember()
	{
		do {
			nCurrentMember++;
			if (nCurrentMember >= nTeamSize) nCurrentMember = 0;
		} while (vecMembers[nCurrentMember]->fHealth <= 0);
		return vecMembers[nCurrentMember];
	}
};




