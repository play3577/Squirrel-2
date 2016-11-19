#pragma once
#include "position.h"
#include "fundation.h"
//ベンチマーク関数

//指し手生成速度計測
void speed_genmove(const Position& pos);

void wrap_randomwalker();

//randomwalker
void randomwalker(Position& pos, int maxdepth);

