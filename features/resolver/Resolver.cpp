#include "../../includes.h"
#include "../../utilities/interfaces.h"
#include "../../sdk/IEngine.h"
#include "../../utilities/render.h"
#include "../../sdk/CUserCmd.h"
#include "../../sdk/CBaseEntity.h"
#include "../../sdk/CClientEntityList.h"
#include "../../sdk/CTrace.h"
#include "../../sdk/CBaseWeapon.h"
#include "../../sdk/CGlobalVars.h"
#include "../../sdk/ConVar.h"
#include "../../sdk/AnimLayer.h"
#include "../../utilities/qangle.h"
#include "../../features/aimbot/Aimbot.h"
#include "../../features/resolver/Resolver.h"
#include "../../utilities/variables.h"

Vector old_calcangle(Vector dst, Vector src) {
	Vector angles;

	double delta[3] = { (src.x - dst.x), (src.y - dst.y), (src.z - dst.z) };
	double hyp = sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
	angles.x = (float)(atan(delta[2] / hyp) * 180.0 / 3.14159265);
	angles.y = (float)(atanf(delta[1] / delta[0]) * 57.295779513082f);
	angles.z = 0.0f;

	if (delta[0] >= 0.0)
	{
		angles.y += 180.0f;
	}

	return angles;
}

float old_normalize(float Yaw) {
	if (Yaw > 180) {
		Yaw -= (round(Yaw / 360) * 360.f);
	}
	else if (Yaw < -180) {
		Yaw += (round(Yaw / 360) * -360.f);
	}
	return Yaw;
}

float curtime(sdk::CUserCmd* ucmd) {
	auto local_player = ctx::client_ent_list->GetClientEntity(ctx::engine->GetLocalPlayer());

	if (!local_player)
		return 0;

	int g_tick = 0;
	sdk::CUserCmd* g_pLastCmd = nullptr;
	if (!g_pLastCmd || g_pLastCmd->hasbeenpredicted) {
		g_tick = (float)local_player->GetTickBase();
	}
	else {
		++g_tick;
	}
	g_pLastCmd = ucmd;
	float curtime = g_tick * ctx::globals->interval_per_tick;
	return curtime;
}

bool find_layer(sdk::CBaseEntity* entity, int act, sdk::CAnimationLayer *set) {
	for (int i = 0; i < 13; i++) {
		sdk::CAnimationLayer layer = entity->GetAnimOverlay(i);
		const int activity = entity->GetSequenceActivity(layer.m_nSequence);
		if (activity == act) {
			*set = layer;
			return true;
		}
	}
	return false;
}

void CResolver::record(sdk::CBaseEntity* entity, float new_yaw) {
	if (entity->GetVelocity().Length2D() > 36)
		return;

	auto c_baseweapon = reinterpret_cast< sdk::CBaseWeapon* >(ctx::client_ent_list->GetClientEntity(entity->GetActiveWeaponIndex()));

	if (!c_baseweapon)
		return;

	auto &info = player_info[entity->GetIndex()];

	if (entity->GetActiveWeaponIndex() && info.last_ammo < c_baseweapon->GetLoadedAmmo()) {
		//ignore the yaw when it is from shooting (will be looking at you/other player)
		info.last_ammo = c_baseweapon->GetLoadedAmmo();
		return;
	}

	info.unresolved_yaw.insert(info.unresolved_yaw.begin(), new_yaw);
	if (info.unresolved_yaw.size() > 20) {
		info.unresolved_yaw.pop_back();
	}

	if (info.unresolved_yaw.size() < 2)
		return;

	auto average_unresolved_yaw = 0;
	for (auto val : info.unresolved_yaw)
		average_unresolved_yaw += val;
	average_unresolved_yaw /= info.unresolved_yaw.size();

	int delta = average_unresolved_yaw - entity->GetLowerBodyYaw();
	auto big_math_delta = abs((((delta + 180) % 360 + 360) % 360 - 180));

	info.lby_deltas.insert(info.lby_deltas.begin(), big_math_delta);
	if (info.lby_deltas.size() > 10) {
		info.lby_deltas.pop_back();
	}
}

void CResolver::Nospread(sdk::CBaseEntity * player, int entID)
{
	auto local_player = ctx::client_ent_list->GetClientEntity(ctx::engine->GetLocalPlayer());

	if (!local_player)
		return;

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	float atTargetAngle = UTILS::CalcAngle(local_player->GetHealth() <= 0 ? local_player->GetVecOrigin() : local_position, player->GetVecOrigin()).y;
	Vector velocityAngle;
	MATH::VectorAngles(player->GetVelocity(), velocityAngle);

	float primaryBaseAngle = player->GetLowerBodyYaw();
	float secondaryBaseAngle = velocityAngle.y;

	switch ((shots_missed[entID]) % 15)
	{
	case 0:
		player->EasyEyeAngles()->yaw = atTargetAngle + 180.f;
		break;
	case 1:
		player->EasyEyeAngles()->yaw = velocityAngle.y + 180.f;
		break;
	case 2:
		player->EasyEyeAngles()->yaw = primaryBaseAngle;
		break;
	case 3:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 45.f;
		break;
	case 4:
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 90.f;
		break;
	case 5:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 130.f;
		break;
	case 6:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 180.f;
		break;
	case 7:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle;
		break;
	case 8:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 40.f;
		break;
	case 9:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 90.f;
		break;
	case 10:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 130.f;
		break;
	case 11:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 70.f;
		break;
	case 12:
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 45.f;
		break;
	case 13:
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 135.f;
		break;
	case 14:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 90.f;
		break;
	case 15:
		player->EasyEyeAngles()->yaw = primaryBaseAngle / 1.1;
		break;
	case 16:
		player->EasyEyeAngles()->yaw = primaryBaseAngle * 1.1;
		break;
	case 17:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle / 1.13;
		break;
	case 18:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle * 1.13;
		break;
	case 19:
		player->EasyEyeAngles()->yaw = atTargetAngle / 1.12;
		break;
	case 20:
		player->EasyEyeAngles()->yaw = atTargetAngle * 1.12;
		break;
	case 21:
		player->EasyEyeAngles()->yaw = atTargetAngle / 1.5;
		break;
	case 22:
		player->EasyEyeAngles()->yaw = atTargetAngle * 1.5;
		break;
	case 23:
		player->EasyEyeAngles()->roll = atTargetAngle * 1.12;
		break;
	}
}



void CResolver::Experimental(sdk::CBaseEntity* entity) {

	auto local_player = ctx::client_ent_list->GetClientEntity(ctx::engine->GetLocalPlayer());

	if (!entity)
		return;

	if (!local_player)
		return;

	bool is_local_player = entity == local_player;
	bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

	if (is_local_player)
		return;

	if (is_teammate)
		return;

	if (entity->GetHealth() <= 0)
		return;

	if (local_player->GetHealth() <= 0)
		return;

	//--- Variable Declaration ---//;
	auto &info = player_info[entity->GetIndex()];

	//--- Variable Defenitions/Checks ---//
	float fl_lby = entity->GetLowerBodyYaw();

	info.lby = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw(), 0.f);
	info.inverse = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw() + 180.f, 0.f);
	info.last_lby = Vector(entity->GetEyeAngles().x, info.last_moving_lby, 0.f);
	info.inverse_left = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw() + 115.f, 0.f);
	info.inverse_right = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw() - 115.f, 0.f);

	info.back = Vector(entity->GetEyeAngles().x, UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y + 180.f, 0.f);
	info.right = Vector(entity->GetEyeAngles().x, UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y + 70.f, 0.f);
	info.left = Vector(entity->GetEyeAngles().x, UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y - 70.f, 0.f);

	info.backtrack = Vector(entity->GetEyeAngles().x, lby_to_back[entity->GetIndex()], 0.f);

	shots_missed[entity->GetIndex()] = shots_fired[entity->GetIndex()] - shots_hit[entity->GetIndex()];

	info.is_moving = entity->GetVelocity().Length2D() > 0.1 && entity->GetFlags() & FL_ONGROUND;
	info.is_jumping = !entity->GetFlags() & FL_ONGROUND;
	info.could_be_slowmo = entity->GetVelocity().Length2D() > 6 && entity->GetVelocity().Length2D() < 36 && !info.is_crouching;
	info.is_crouching = entity->GetFlags() & FL_DUCKING;
	update_time[entity->GetIndex()] = info.next_lby_update_time;

	static float old_simtime[65];
	if (entity->GetSimTime() != old_simtime[entity->GetIndex()])
	{
		using_fake_angles[entity->GetIndex()] = entity->GetSimTime() - old_simtime[entity->GetIndex()] == ctx::globals->interval_per_tick; //entity->GetSimTime() - old_simtime[entity->GetIndex()] >= TICKS_TO_TIME(2)
		old_simtime[entity->GetIndex()] = entity->GetSimTime();
	}

	//--- Actual Angle Resolving ---//
	if (!using_fake_angles[entity->GetIndex()])
	{
		if (backtrack_tick[entity->GetIndex()])
		{
			resolve_type[entity->GetIndex()] = 7;
			entity->SetEyeAngles(info.backtrack);
		}
		else if (info.stored_lby != entity->GetLowerBodyYaw() || entity->GetSimTime() > info.next_lby_update_time) //lby prediction
		{
			entity->SetEyeAngles(info.lby);
			info.next_lby_update_time = entity->GetSimTime() + 1.1;
			info.stored_lby = entity->GetLowerBodyYaw();
			resolve_type[entity->GetIndex()] = 3;
			entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.1 + 120 * 2 / 1.23 + 22;
			entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.2 * 1.1;
			entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.4 * 1.1;
			entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().z * 1.01;
			entity->EasyEyeAngles()->roll = entity->GetEyeAngles().z * 1.01;
		}
		else if (info.is_jumping)
		{
			Nospread(entity, entity->GetIndex());
		}
		else if (info.is_moving) //while moving
		{
			entity->SetEyeAngles(info.lby);
			info.last_moving_lby = entity->GetLowerBodyYaw();
			info.stored_missed = shots_missed[entity->GetIndex()];
			resolve_type[entity->GetIndex()] = 1;
			entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.1 + 4;
			entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.34 * 0.53 + 27 / 1.32 * 1.63;
			entity->SetEyeAngles(info.last_lby);
			entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().x;
			entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().z * 1.01;
			entity->EasyEyeAngles()->roll = entity->GetEyeAngles().z * 1.01;
			entity->EasyEyeAngles()->roll = entity->GetEyeAngles().z * 1.03;
			entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().y * 2;
		}
		else
		{
			if (shots_missed[entity->GetIndex()] > info.stored_missed) //if we have missed 1 shot since we have stopped moving
			{
				resolve_type[entity->GetIndex()] = 4;
				switch (shots_missed[entity->GetIndex()] % 4)
				{
				case 0: entity->SetEyeAngles(info.inverse); break;
				case 1: entity->SetEyeAngles(info.left); break;
				case 2: entity->SetEyeAngles(info.back); break;
				case 3: entity->SetEyeAngles(info.right); break;
				case 4: entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.2 * 0.3; break;
				case 5: entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.4 + 5 * 0.32 / 0.54; break;
				case 6: entity->SetEyeAngles(info.last_lby); break;
				case 7: entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() + 40 * 0.2 / 0.13 + 4; break;
				case 8: entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 2 * 1.3 - 40; break;
				case 9: entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().z * 1.01; break;
				case 10: entity->EasyEyeAngles()->pitch = entity->GetEyeAngles().x; break;
				case 11: entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().z * 1.02; break;
				case 12: entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().y * 2; break;
				}
			}
			else //first thing we shoot when they stop
			{
				entity->SetEyeAngles(info.last_lby);
				resolve_type[entity->GetIndex()] = 5;
				entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.1 + 4 * 0.2 + 16 * 0.3;
				entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() + 4 / 1.1 * 0.9 + 20 / 1.6;
				entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.1 + 4;
				entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().x;
				entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().z * 1.01;
				entity->EasyEyeAngles()->roll = entity->GetEyeAngles().z * 1.01;
				entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().y * 2;
				}
			}
			if (resolve_type[entity->GetIndex()] = 5)
			{
				entity->SetEyeAngles(info.last_lby);
				resolve_type[entity->GetIndex()] = 5;
				entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.2 * 0.2 / 1.1 + 42 * 1.2 + 62 / 1.2 * 1.3;
				entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() * 1.1 + 3 / 1.54;
				entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().z * 1.01;
				entity->EasyEyeAngles()->pitch = entity->GetEyeAngles().x;
				entity->EasyEyeAngles()->roll = entity->GetEyeAngles().z * 1.01;
				entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().y * 2;
			}
			else
			{
				entity->SetEyeAngles(info.last_lby);
				resolve_type[entity->GetIndex()] = 5;
				entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.4 * 0.5 / 2 * 1.2 + 120 * 2.5 + 3 / 1.2 * 1.1 + 42 / 2 * 1.34;
				entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() + 2;
				entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() - 2;
				entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().z * 1.01;
				entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().y * 2;
			}
			if (info.breaking_lby)
			{
				entity->SetEyeAngles(info.last_lby);
				entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() + 120 * 2 + 30 / 1.1 * 1.5;
				resolve_type[entity->GetIndex()] = 5;
				entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.3;
				entity->EasyEyeAngles()->roll = entity->GetEyeAngles().z * 1.03;
				entity->EasyEyeAngles()->roll = entity->GetLowerBodyYaw() / 1.3 + 30 * 1.2;
				entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().y * 2;
			}
			else
			{
				if (entity->GetSimTime() + 1.1)
				{
					entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.4 + 32 * 1.2;
					entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().y * 2;
					entity->SetEyeAngles(info.last_lby);
					entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.2 * 0.1;
					entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().z * 1.01;
					entity->EasyEyeAngles()->roll = entity->GetEyeAngles().z * 1.01;
					resolve_type[entity->GetIndex()] = 5;
					entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw();
				}
				else
				{
					entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.4 * 0.2;
					entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.6 * 0.3;
					entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.8 * 0.4;
					entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().z * 1.01;
					entity->EasyEyeAngles()->roll = entity->GetEyeAngles().z * 1.01;
					entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().y * 2;
					entity->SetEyeAngles(info.last_lby);
					resolve_type[entity->GetIndex()] = 5;
					for (int w = 0; w < 13; w++)
					{
						entity->GetAnimOverlay(w);
						resolve_type[entity->GetIndex()] = 5;
						entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 2 * 1.5 + 40;
						entity->SetEyeAngles(info.last_lby);
						entity->EasyEyeAngles()->roll = entity->GetEyeAngles().z * 1.01;
						entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() * 1.32;
					}
					if (GLOBAL::bFakewalking = true)
					{
						for (int w = 0; w < 13; w++) {
							entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.32 + 4;
							entity->EasyEyeAngles()->roll = entity->GetEyeAngles().z * 1.03;
							entity->GetAnimOverlay(w);
						}
					}
					else
					{
						entity->EasyEyeAngles()->roll = entity->GetLowerBodyYaw() * 1.3 + 24 / 1.2;
					}
				}
			}
		}
	}

bool IsFakeWalking(sdk::CBaseEntity* entity) {
	for (int s = 0; s < 13; s++) {
		auto anim_layer = entity->GetAnimOverlay(s);
		if (!anim_layer.m_pOwner)
			continue;

		auto activity = entity->GetSequenceActivity(anim_layer.m_nSequence);
		GLOBAL::bFakewalking = false;
		bool stage1 = false,			// stages needed cause we are iterating all layers, otherwise won't work :)
			stage2 = false,
			stage3 = false;

		for (int i = 0; i < 16; i++) {
			if (activity == 26 && anim_layer.m_flWeight < 0.4f)
				stage1 = true;
			if (activity == 7 && anim_layer.m_flWeight > 0.001f)
				stage2 = true;
			if (activity == 2 && anim_layer.m_flWeight == 0)
				stage3 = true;
			entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.17;
			entity->EasyEyeAngles()->roll = entity->GetEyeAngles().z * 1.03;
		}

		if (stage1 && stage2)
			GLOBAL::bFakewalking = true;
		else
			GLOBAL::bFakewalking = false;

		return GLOBAL::bFakewalking;
	}
}

#define TICK_INTERVAL			(ctx::globals->interval_per_tick)
int latest_tick;
#define TIME_TO_TICKS( dt )		( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )

bool IsTickValid(int tick) {
	int delta = latest_tick - tick;
	float deltaTime = delta * ctx::globals->interval_per_tick;
	return (fabs(deltaTime) <= 0.1f);
}

void CResolver::Default(sdk::CBaseEntity* entity) {

	auto local_player = ctx::client_ent_list->GetClientEntity(ctx::engine->GetLocalPlayer());

	if (!entity)
		return;

	if (!local_player)
		return;

	bool is_local_player = entity == local_player;
	bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

	if (is_local_player)
		return;

	if (is_teammate)
		return;

	if (entity->GetHealth() <= 0)
		return;

	if (local_player->GetHealth() <= 0)
		return;

	//--- Variable Declaration ---//;
	auto &info = player_info[entity->GetIndex()];

	//--- Variable Defenitions/Checks ---//
	float fl_lby = entity->GetLowerBodyYaw();

	info.lby = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw(), 0.f);
	info.inverse = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw() + 180.f, 0.f);
	info.last_lby = Vector(entity->GetEyeAngles().x, info.last_moving_lby, 0.f);
	info.inverse_left = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw() + 115.f, 0.f);
	info.inverse_right = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw() - 115.f, 0.f);

	info.back = Vector(entity->GetEyeAngles().x, UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y + 180.f, 0.f);
	info.right = Vector(entity->GetEyeAngles().x, UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y + 70.f, 0.f);
	info.left = Vector(entity->GetEyeAngles().x, UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y - 70.f, 0.f);

	info.backtrack = Vector(entity->GetEyeAngles().x, lby_to_back[entity->GetIndex()], 0.f);

	shots_missed[entity->GetIndex()] = shots_fired[entity->GetIndex()] - shots_hit[entity->GetIndex()];

	info.is_moving = entity->GetVelocity().Length2D() > 0.1 && entity->GetFlags() & FL_ONGROUND;
	info.is_jumping = !entity->GetFlags() & FL_ONGROUND;
	info.could_be_slowmo = entity->GetVelocity().Length2D() > 6 && entity->GetVelocity().Length2D() < 36 && !info.is_crouching;
	info.is_crouching = entity->GetFlags() & FL_DUCKING;
	update_time[entity->GetIndex()] = info.next_lby_update_time;

	static float old_simtime[65];
	if (entity->GetSimTime() != old_simtime[entity->GetIndex()])
	{
		using_fake_angles[entity->GetIndex()] = entity->GetSimTime() - old_simtime[entity->GetIndex()] == ctx::globals->interval_per_tick; //entity->GetSimTime() - old_simtime[entity->GetIndex()] >= TICKS_TO_TIME(2)
		old_simtime[entity->GetIndex()] = entity->GetSimTime();
	}

	//--- Actual Angle Resolving ---//
	if (!using_fake_angles[entity->GetIndex()])
	{
		if (backtrack_tick[entity->GetIndex()])
		{
			resolve_type[entity->GetIndex()] = 7;
			entity->SetEyeAngles(info.backtrack);
		}
		else if (info.stored_lby != entity->GetLowerBodyYaw())// || entity->GetSimTime() > info.next_lby_update_time) //lby prediction
		{
			entity->SetEyeAngles(info.lby);
			//info.next_lby_update_time = entity->GetSimTime() + 1.1;
			info.stored_lby = entity->GetLowerBodyYaw();
			resolve_type[entity->GetIndex()] = 3;
		}
		else if (info.is_jumping)
		{
			Nospread(entity, entity->GetIndex());
		}
		else if (info.is_moving) //while moving
		{
			entity->SetEyeAngles(info.lby);
			info.last_moving_lby = entity->GetLowerBodyYaw();
			info.stored_missed = shots_missed[entity->GetIndex()];
			resolve_type[entity->GetIndex()] = 1;
			entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().y / 1.07;
		}
		else
		{
			if (shots_missed[entity->GetIndex()] > info.stored_missed) //if we have missed 1 shot since we have stopped moving
			{
				resolve_type[entity->GetIndex()] = 4;
				switch (shots_missed[entity->GetIndex()] % 4)
				{
				case 0: entity->SetEyeAngles(info.inverse); break;
				case 1: entity->SetEyeAngles(info.left); break;
				case 2: entity->SetEyeAngles(info.back); break;
				case 3: entity->SetEyeAngles(info.right); break;
				}
			}
			else //first thing we shoot when they stop
			{
				entity->SetEyeAngles(info.last_lby);
				resolve_type[entity->GetIndex()] = 5;
			}
			if (resolve_type[entity->GetIndex()] = 5)
			{
				entity->SetEyeAngles(info.last_lby);
				resolve_type[entity->GetIndex()] = 5;
				entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.2;
			}
			else
			{
				entity->SetEyeAngles(info.last_lby);
				resolve_type[entity->GetIndex()] = 5;
				entity->EasyEyeAngles()->yaw = entity->GetLowerBodyYaw() / 1.4 * 0.5 + 2;
				entity->EasyEyeAngles()->yaw = entity->GetEyeAngles().z * 1.01;
			}
		}
	}
}

void CResolver::resolve(sdk::CBaseEntity* entity) {

	auto local_player = ctx::client_ent_list->GetClientEntity(ctx::engine->GetLocalPlayer());

	if (!entity)
		return;

	if (!local_player)
		return;

	bool is_local_player = entity == local_player;
	bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

	if (is_local_player)
		return;

	if (is_teammate)
		return;

	if (entity->GetHealth() <= 0)
		return;

	if (local_player->GetHealth() <= 0)
		return;





	//--- Actual Angle Resolving ---//
	switch (Vars::options.res_type) {
	case 0:
		CResolver::Default(entity);
		break;
	case 1:
		CResolver::Experimental(entity);
		break;

	}
}

CResolver* resolver = new CResolver();