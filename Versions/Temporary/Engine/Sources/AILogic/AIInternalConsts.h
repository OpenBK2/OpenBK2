#pragma once
#include "AIConsts.h"
#include "../Misc/HashFuncs.h"


namespace NDb
{
	enum EDesignUnitType;
	struct SUnitTypePriority;
}
struct SConsts : public SAIConsts
{
private:
	static void LoadRevealInfo();
public:
	// plane path
	static float PLANE_SPLINE_STEP;
	static float PLANE_SPLINE_POINT_DIST;
	static int PLANE_TARGET_SCAN_PERIOD;

	struct SCamouflage
	{
		static const float SNEAK_ADDITION_PER_TIME;			// addition to camouflage when idle
		static const float SNEAK_DECREASE_PER_SHOT;			// when shot - decrease camouflage
		static const float SNEAK_DECREASE_PER_SECOND_MOVE;	// when camoflated unit moves, it's camoflage decresed with this rate
		static const int SNEAK_ADVANCED_TIMEOUT;
		static const int SNEAK_ADVANCED_RESET_TIMEOUT;
	};
	// расстояние, в котором пересчитать туман для юнитов при удалении стат. объекта
	static const int MAX_DIST_TO_RECALC_FOG;
	
	// на какой максимальный угол поворачиваться мгновенно
	static const int TURN_TOLERANCE;
	
	// стандартный угол обзора
	static const WORD STANDART_VIS_ANGLE;
	
	// при какой разнице в угле нужно остановиться и поворачиваться
	static const WORD DIR_DIFF_TO_SMOOTH_TURNING;

	// максимальная длина пути в тайлах, при котором можно ехать задом
	static const int MAX_LEN_TO_GO_BACKWARD;
	
	// насколько forward iteration смотреть вперёд при хождении вдоль сплайна
	static const int NUMBER_ITERS_TO_LOOK_AHEAD;
	
	// для прямоугольника впереди движущегося юнита
	static const int SPEED_FACTOR;

	// сколько отмерять тайлов пути при построении сплайна
	static const short int SPLINE_STEP;

	// size of "neighbours scan" cell
	static const int CELL_COEFF;
	static const int CELL_SIZE;			// must be divisible by TILE_SIZE
	
	static const int BIG_CELL_COEFF;
	static const int BIG_CELL_SIZE;
	
	// max number of tiles, occupied by a unit
	static const int MAX_UNIT_TILE_RADIUS;
	// максимальный радиус юнита в точках
	static const int MAX_UNIT_RADIUS;
	
	static const int BIG_PATH_SHIFT;

	// starting sizes of vectors
	static const int AI_START_VECTOR_SIZE;
	
	//
	// максимальное асстояние для того, чтобы юниты были в одной "простой" группе
	static const int GROUP_DISTANCE;

	// периоды сканирования при поиске ближайшего врага для antiartillery борьбы
	static const float ANTI_ARTILLERY_SCAN_TIME;

	// коэффициент для утолщения/удлинения boundRect
	static const float BOUND_RECT_FACTOR;
	// коэффициент для утолщения/удлинения boundRect при локании тайлов
	static const float COEFF_FOR_LOCK;
	// расстояние от точки высадки транспорта до точки, куда оттуда нужно сразу же бежать ( для поиска пути )
	static const float DIST_FOR_LAND;
	// расстояние от точки высадки транспорта до точки, куда оттуда нужно сразу же бежать ( для проверки "пригодности" точки )
	static float GOOD_LAND_DIST;
	
	//
	// деление карты на ячейки для статических объектов
	static const int STATIC_OBJ_CELL;
	static const int STATIC_CONTAINER_OBJ_CELL;
	
	// размер ячейки для записи разорвавшихся снарядов
	static const int HIT_CELL_COEFF;
	static const int HIT_CELL_SIZE;

	// количество направлений у солдатиков
	static const int NUMBER_SOLDIER_DIRS;

	// константы, считываетмые из внешнего файла
	//
	// скорость лечения в зданиях HP / tick
	static float CURE_SPEED_IN_BUILDING;
	// время между концом очереди и началом возвращения turret в default position
	static int TIME_TO_RETURN_GUN;
	// количество сканируемых за сегмент
	static int NUM_TO_SCAN_IN_SEGM;
	// время между updates поведения
	static int BEH_UPDATE_DURATION;
	// время между updates поведения для солдат
	static int SOLDIER_BEH_UPDATE_DURATION;
	// то же для AA 
	static int AA_BEH_UPDATE_DURATION;
	// для дальнобойной артиллерии
	static int LONG_RANGE_ARTILLERY_UPDATE_DURATION;
	// время, которое мертвецы видят
	static int DEAD_SEE_TIME;
	// время, в течении которого тревога в здании
	static int TIME_OF_BUILDING_ALARM;
	// время, через которое нужно камуфлироваться в состоянии idle
	static int TIME_BEFORE_CAMOUFLAGE;
	// время лежания под обстрелом
	static int TIME_OF_LYING_UNDER_FIRE;
	// cover для лежащих - вероятность, что попадут
	static float LYING_SOLDIER_COVER;
	// радиус в точках , который опрашивается, чтобы узнать, под обстрелом или нет
	static int RADIUS_OF_HIT_NOTIFY;
	// периодичность проверки, под обстрелом или нет
	static int TIME_OF_HIT_NOTIFY;
	
	// время в милисекундах между 2 сканированиями мин для инженеров
	static int ENGINEER_MINE_CHECK_PERIOD; 
	// радиус ( в точках ), в котором инженер видит мины
	static int MINE_VIS_RADIUS;
	// радиус ( в точках ), в котором инженер снимает мины
	static int MINE_CLEAR_RADIUS;
	// максимальное расстояние убегания от центра формации
	static int RADIUS_OF_FORMATION;
	
	// радиус, в котором можно бегать при guard state
	static float GUARD_STATE_RADIUS;
	
	// множитель на скорость при ползании
	static float LYING_SPEED_FACTOR;
	
	// радиус для call for help
	static int CALL_FOR_HELP_RADIUS;
	static int AI_CALL_FOR_HELP_RADIUS;
	
	// время, которое солдаты собираются в здании перед участием в штурме
	static int CAMPING_TIME;
	// множитель на weapon range при стрельбе солдатиков внутри объекта
	static float INSIDE_OBJ_WEAPON_FACTOR;
	// период времени, с которым комбатятся два солдата внутри объекта
	static int INSIDE_OBJ_COMBAT_PERIOD;
	// время, через которое техника исчезает после смерти
	static int TIME_TO_DISAPPEAR;

	// пороговое время времени install/uninstall для того, чтобы пушки install/uninstall самостоятельно
	static int THRESHOLD_INSTALL_TIME;
	
	// количество выстрелов для пристрелки артиллерии по области
	static int SHOOTS_TO_RANGE;
	// коэффициент на dispersion если ведётся стрельба по пристрелянной области
	static float RANDGED_DISPERSION_RADIUS_BONUS;
	// радиус области пристрелки
	static float RANGED_AREA_RADIUS;

	// расстояние, на которое нужно сместиться артиллерии для того, чтобы сбросить info о её местонахождении
	static float RELOCATION_RADIUS;
	// макс. радиус кружочка вокруг стреляющей артиллерии
	static float MAX_ANTI_ARTILLERY_RADIUS;
	// мин. радиус кружочка вокруг стреляющей артиллерии
	static float MIN_ANTI_ARTILLERY_RADIUS;
	// количество выстрелов, чтобы свести MAX_ANTI_ARTILLERY_RADIUS к MIN_ANTI_ARTILLERY_RADIUS
	static int SHOTS_TO_MINIMIZE_LOCATION_RADIUS;
	// время, которое рисуются круги после выстрела пушки
	static int AUDIBILITY_TIME;
	// периодичность рисования кругов антиартиллерийской борьбы
	static int REVEAL_CIRCLE_PERIOD;
	
	// вероятность пробивания брони, при которой не нужно маневрировать
	static float GOOD_ATTACK_RPOBABILITY;
	
	//for fighter
	static int FIGHTER_PATROL_TIME;//after that time fighter will cancel patrolling 
	static float FIGHTER_VERTICAL_SPEED_UP;		// скорость набора высоты
	static float FIGHTER_VERTICAL_SPEED_DOWN;		// скорость сброса высоты
	
	//for paratrooper
	static float PARATROOPER_FALL_SPEED;
	static int PARADROP_SPRED ; //для поиска свободного тайла, на который выпрыгнуть.

	//радиус в котором инженеры ищут юниты, которые будут обеспечены патронами из грузовичка
	static int RESUPPLY_RADIUS;
	// для морали
	static int RESUPPLY_RADIUS_MORALE;

	static int TIME_QUANT;		//time of quant repear operation
	static float RU_PER_QUANT;	// resurs units flow per quant (used in all engineers operations)
	static float ENGINEER_RU_CARRY_WEIGHT;						// при загрузке грузовичка инженеры носят столько RU
	static int COMPLEX_OBSTACLE_ANTITANK_PERIOD;

	// радиус обзора бинокля
	static float SPY_GLASS_RADIUS;
	// угол обзора бинокля
	static WORD SPY_GLASS_ANGLE;
	static float HOLD_SECTOR_SIGHT_BONUS;
	static float HOLD_SECTOR_DISPERSION_BONUS;
	static int TIME_TO_ENTER_HOLD_SECTOR;
	
	static float TRACK_TARGETING_AIM_BONUS;
	static float TRACK_TARGETING_DAMAGE_MODIFIER;

	// коэффициент на area damage
	static float AREA_DAMAGE_COEFF;
	// минимальный угол, на который можно повернуть базу во время атаки turret-ом, чтобы улучшить свою позицию
	static WORD MIN_ROTATE_ANGLE;
	
	// радиус кругов от выстрелов, начиная с которого артиллерия начинает antiartillery борьбу
	static float RADIUS_TO_START_ANTIARTILLERY_FIRE;
	
	// емкость грузовика в RU
	static float TRANSPORT_RU_CAPACITY;
	// дистанция на которую дожен подъехать грузовик к складу
	static float TRANSPORT_LOAD_RU_DISTANCE ;
	// максимальная длина пути (от склада, в тайлах), по которой происходит снабжение
	static int RESUPPLY_MAX_PATH;
	
	// время, которое держится alarm при обстреле юнита снарядами по баллистиике
	static float TIME_OF_ALARM_UNDER_FIRE;
	// радиус действия для складов. на такое расстояние телепортятся ресурсы между сладами
	static float STORAGE_RESUPPLY_RADIUS;
	
	// сопротивление воздуха для бомб
	static float TRAJ_BOMB_ALPHA;
	
	// подойдя на это расстояние к нужной точке артиллеристы телепортятся в нее.
	static float GUN_CREW_TELEPORT_RADIUS;

	//  квадрат расстояния между выбросами паращютистов
	static float PLANE_PARADROP_INTERVAL;
	// максимальное смещение паращютисов перпендикулярно вектору движения самолетов.
	static float PLANE_PARADROP_INTERVAL_PERP_MIN;
	static float PLANE_PARADROP_INTERVAL_PERP_MAX;
	
	// fuel control
	static float PLANE_FUEL_DEC_ECONOMY;
	static float PLANE_FUEL_DEC;

	// 1 раз в этот интервал паращютисты проверяют не падают ли они на залоченную клетку
	static int PARATROOPER_GROUND_SCAN_PERIOD;

	// с этого расстояния грузовичок за пушкой будет подъезжать задом	
	static float TRANSPORT_MOVE_BACK_DISTANCE;

	// столько раз транспорт будет пытаться отцепить артиллерию.
	static int TRIES_TO_UNHOOK_ARTILLERY;

	static float PLANE_GUARD_STATE_RADIUS;
	static float PLANES_HEAVY_FORMATION_SIZE;
	static float PLANES_SMALL_FORMATION_SIZE;
	static float PLANES_START_RANDOM;

	// здоровье гусеницы
	static float TANK_TRACK_HIT_POINTS;
	
	// какую долю расстояния снаряд по пологой траектории летит прямо
	static float TRAJECTORY_LOW_LINE_RATIO;

	static float TRAJECTORY_BOMB_G;

	// радиус выхода из пикирования на цель
	static float PLANE_DIVE_FINISH_DISTANCE_SQR	;
	
	static float SNIPER_CAMOUFLAGE_DECREASE_PER_SHOOT;
	static float SNIPER_CAMOUFLAGE_INCREASE;
	
	// доля юнитов, которые должны иметь возможность поразить
	// цель из засады для того, чтобы весь Ambush начал атаковать.
	static float AMBUSH_ATTACK_BEGIN_CIRTERIA;
	// это значение делится на revealRadous и умножается на AntiArtilleryRadius
	static float ARTILLERY_REVEAL_COEEFICIENT;
	// минимальный и максимальный разброс для всех типов траекторий
	static float dispersionRatio[9][2];
	
	static float COEFF_FOR_RANDOM_DELAY;
	
	// на какую высоту нужно подняться, чтобы радиус видимости увеличился на один тайл
	static float HEIGHT_FOR_VIS_RADIUS_INC;
	
	// скорость горения здания ( проценты в tick )
	static float BURNING_SPEED;
	
	// follow константы
	// радиус достаточно близко от ведущего
	static float FOLLOW_STOP_RADIUS;
	// радиус в котором нужно приравнять скорость к скорости ведущего
	static float FOLLOW_EQUALIZE_SPEED_RADIUS;
	// радиус, из-за которого нужно ехать за ведущим
	static float FOLLOW_GO_RADIUS;
	// радиус, за которым ведущий должен ждать
	static float FOLLOW_WAIT_RADIUS;
	
	static float FATALITY_PROBABILITY;
	static float DAMAGE_FOR_MASSIVE_DAMAGE_FATALITY;
	static float MASSIVE_DAMAGE_FATALITY_PROBABILITY;

	static float BOMB_START_HEIGHT;
	
	static float STAND_LIE_RANDOM_DELAY;
	
	static float TRANSPORT_RESUPPLY_OFFSET;
	static float HP_BALANCE_COEFF;
	
	// при выходе из дома солдатики в squad будут выходить по одному через этот интервал
	static int SQUAD_MEMBER_LEAVE_INTERVAL;

	// стоимость солдата в RU. нужна при восстановлении 
	static float SOLDIER_RU_PRICE;
	static float LOW_HP_PERCENTAGE;
	
	// damage по прямой траектории в отрезок времени, необходимый для того, чтобы ситуация была комбатная
	static float DIRECT_HIT_DAMAGE_COMBAT_SITUATION;
	static int DIRECT_HIT_TIME_COMBAT_SITUATION;
	static int NUMBER_ENEMY_MECH_MOVING_TO_COMBAT_SITUATION;
	static int NUMBER_ENEMY_INFANTRY_MOVING_TO_COMBAT_SITUATION;
	
	// коэффициент на firerange оружия офицера, в котором он сканирует ( радиус сканирования firerange * OFFICER_COEFFICIENT_FOR_SCAN )
	static float OFFICER_COEFFICIENT_FOR_SCAN;
	
	static float MAIN_STORAGE_HEALING_SPEED;
	static float RADIUS_TO_TAKE_STORAGE_OWNERSHIP;
	static float TANKPIT_COVER;
	static const float CLOSEST_TO_RAILROAD_POINT_TOLERANCE;

	static float FENCE_SEGMENT_RU_PRICE;
	static float ENTRENCHMENT_SEGMENT_RU_PRICE;
	static float MINE_RU_PRICE[2];
	static float ANTITANK_RU_PRICE;
	
	// время, которое юнит виден после того, как он уехал из области видимости
	static int RESIDUAL_VISIBILITY_TIME;

	static int MED_TRUCK_HEAL_RADIUS;
	static float MED_TRUCK_HEAL_PER_UPDATEDURATION;
	
	//
	// периодичность, с которой юнит пытается найти путь до своей формации, если он где-то заблокался
	static int PERIOD_OF_PATH_TO_FORMATION_SEARCH;
	
	static int ENTRENCH_SELF_TIME;
	
	// максимальное количество юнитов, которое может быть просканировано за сегмент
	static int N_SCANNING_UNITS_IN_SEGMENT;

	// size of cell for general
	static int GENERAL_CELL_SIZE;
	
	// flags
	// радиус зоны флага (в точках)
	static float FLAG_RADIUS;
	// время, чтобы захватить флаг ( в секундах )
	static float FLAG_TIME_TO_CAPTURE;
	// очки за флаг в секунду
	static float FLAG_POINTS_SPEED;
	// "флаговые очки", которые всегда идут, независимо от присутствия флагов
	static float PLAYER_POINTS_SPEED;
	// количество очков, чтобы пришло подкрепление
	static float FLAG_POINTS_TO_REINFORCEMENT;

	//
	// максимальная броня, пробиваемая area damage
	static int ARMOR_FOR_AREA_DAMAGE;
	
	// default cover for fireplace
	static float BUILDING_FIREPLACE_DEFAULT_COVER;

	static int DIVE_BEFORE_EXPLODE_TIME;
	static int DIVE_AFTER_EXPLODE_TIME;
	

	static int WEATHER_TIME;
	static int WEATHER_TIME_RANDOM;
	static int WEATHER_TURN_PERIOD;
	static int WEATHER_TURN_PERIOD_RANDOM;
	
	static float BAD_WEATHER_FIRE_RANGE_COEFFICIENT;
	static int TIME_TO_WEATHER_FADE_OFF;
	
	static int AA_AIM_ITERATIONS;
	
	// во сколько раз быстрее понижать мораль без офицера
	static float COEFF_TO_LOW_MORALE_WITHOUT_OFFICER;
	static float MORALE_DISPERSION_COEFF;
	static float MORALE_RELAX_COEFF;
	static float MORALE_AIMING_COEFF;

	// максимальное расстояние, чтобы солдат пытался бросить гранату
	static float MAX_DISTANCE_TO_THROW_GRENADE;
	
	// target resolution constants
	static float TR_GUNPLANE_ALPHA_ATTACK_1;//1.0f;
	static float TR_GUNPLANE_ALPHA_ATTACK_2;//0.3f;
	static float TR_GUNPLANE_ALPHA_GO;//0.005f;
	static float TR_GUNPLANE_ALPHA_KILL;//1.0f;
	static float TR_GUNPLANE_ALPHA_PRICE;//1.0f;
	static float TR_GUNPLANE_LIMIT_TIME; //1000
	static float TR_DISTANCE_TO_CENTER_FACTOR;

	static float MAX_FIRE_RANGE_TO_SHOOT_BY_LINE;

	static int SHOW_ALL_TIME_COEFF;
	
	static float HP_PERCENT_TO_ESCAPE_FROM_BUILDING;

	static float REINFORCEMENT_GROUP_DISTANCE;
	static float REPAIR_COST_ADJUST ;

	static int TIME_TO_ALLOW_KILL_CRUSHED_OBJ;
	//static float TILE_HEIGHT_DIFF_TO_LOCK;
	static float ARTILLERY_CHECKFREE_PERIOD;
	static float FORMATION_CHECKFREEART_PERIOD;

	struct SRevealInfo
	{
		// вероятность раскрытия юнита, когда он кого-то обстреливает (бросается раз в определённое время, 1-2 секунд)
		float fRevealByQuery;
		// вероятность исчезновения раскрытия по прошествию некоторого время без обстрела
		float fRevealByMovingOff;
		// расстояние, на котором раскрытие сбрасывается при удалении от точки обстрела
		float fForgetRevealDistance;
		// время, через которое раскрытие сбрасывается без обстрела
		int nTimeOfReveal;

		SRevealInfo() : fRevealByQuery( 0.0f ), fRevealByMovingOff( 0.0f ), fForgetRevealDistance( 0.0f ), nTimeOfReveal( 0 ) { }
	};
	static hash_map<int, SRevealInfo> REVEAL_INFO;

	// infantry will be healed to full health during this time
	static NTimer::STime INFANTRY_FULL_HEAL_TIME;
	static NTimer::STime STORAGE_CAPTURE_TIME;
	//
	static void Load();
	//
	static hash_map< NDb::EDesignUnitType, int, SEnumHash > PRIORITIES;
	static const int GetUnitPriority( const NDb::EDesignUnitType &unitType )
	{
		hash_map< NDb::EDesignUnitType, int, SEnumHash >::const_iterator pos = PRIORITIES.find( unitType );
		if ( pos == PRIORITIES.end() )
			return 0;
		else
			return pos->second;
	}
  static void InitPriorities( const vector<NDb::SUnitTypePriority> &priorities );
};

