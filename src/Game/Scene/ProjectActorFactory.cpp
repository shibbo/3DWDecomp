#include "Scene/ProjectActorFactory.hpp"

class AllDeadWatcher;
class DisasterBlockDeadWatcher;
class AssistSlideMapParts;
class AssistSlideMapPartsGroup;
class BalanceTruck;
class Ball;
class BallGimmick;
class BallSnow;
class BallYarn;
class BgmPlayObj;
class BgmStopObj;
class BgmRegionChanger;
class Bird;
class BlockAssistLeaf;
class BlockBrick;
class BlockBrickBig;
class BlockBrickBreakable;
class BlockBrickBreakableCourseSelect;
class BlockChoiceWatcher;
class BlockEmpty;
class BlockEmptyCourseSelect;
class BlockHard;
class BlockHardLaserOnly;
class BlockPow;
class BlockQuestion;
class BlockQuestionCourseSelect;
class BlockRoulette;
namespace al { class BlockRail; }
class BlockRailMover;
class BlockSlot;
class BlockSwitch;
class BlockTransparent;
class BobsledDashPanel;
class Bomb;
class BombBound;
class BombBoundGenerator;
class BombHei;
class BombHeiLauncher;
class Bonze;
class BoomerangBros;
class BoomerangFlower;
class BossBunretsu;
class BossGorobon;
class BossWackun;
class BoxCoin;
class BoxKiller;
class BoxKillerLauncher;
class BoxKuribo;
class BoxLight;
class BreakMapParts;
class Bubble;
class Bull;
class Bunbun;
class Bush;
class Byugo;
class CameraInSwitchOnAreaWatcher;
class CameraLookAtPoint;
namespace al { class CameraRailHolder_RS; }
class CameraRailObserver;
class CameraWall;
class Candlestand;
class CandlestandWatcher;
class CatGull;
class CheckPoint;
class CheckpointFlag;
class ChikaChikaBlockWatcher;
class ChikuwaBlock;
class ChorobonColony;
class ChorobonCubeHolder;
class ChorobonHolder;
class ClimbHandle;
class ClimbHandleMapParts;
namespace al { class ClockMapParts; }
class CloudBonusLauncher;
class Coin;
class CoinBlowConcentric;
class CoinBlowGenerator;
class CoinChameleon;
class CoinChameleonCourseSelect;
class CoinCirclePlacement;
class CoinCollectWatcher;
class CoinConcentricCircle;
class CoinCourseSelect;
class CoinFallGenerator;
class CoinFalls;
class CoinLine;
class CoinRail;
class CoinRedRing;
class CoinRing;
class CoinStack;
class CoinStackMoving;
class CollectRingHolder;
class CollisionSearchObj;
namespace al { class ConveyerMapParts; }
class CourseSelectBgmEventController;
class CourseSelectCloud;
class CourseSelectDokan;
class CourseSelectMiniature;
class CourseSelectNode;
class CourseSelectRhythmObject;
class CourseSelectRocket;
class CourseSelectRouteDokan;
class CourseSelectWall;
class CourseSelectWorldWarpDokan;
class Crab;
class Crawler;
class CrawlerGenerator;
class CubeMapController;
class DarkBowser;
class DarkBowserLaserIndicator;
class DashPanel;
class DestructableMapParts;
class DisasterFixMapParts;
class DemoEventGateKeeperChecker;
class DemoAnimatic;
class DemoCutscene;
class DemoKoopaW7;
class DemoObjBase;
class DemoOpeningSwitch;
class DemoStartPosition;
class DemoTimerStageSwitchController;
class DisasterModeController;
class DisasterSpike;
class DisasterSpikeBouncy;
namespace al { class DirectionalLightRequester; }
class Dokan;
class DoorKey;
class DoorLock;
class DokanWorldWarp;
class Donketsu;
class Dossun;
class DoubleMario;
class EchoBlockMapParts;
namespace rc { class EffectObjGame; }
class EffectObjBattleArenaBunBun;
namespace rc { class EffectObjFollowCameraGame; }
class EnemyGenerator;
class EnterCatMarioMiddleViewRocket;
namespace al { class FallMapParts; }
class FairyHouseIllustItemWatcher;
class FairyMii;
class FairyNpc;
class FairyNpcWithGlasses;
class FairyPrincess;
class FallingPillar;
class FieryRotateParts;
class FireBarRoot;
class FireBros;
class FireFlower;
class Fireworks;
class FireworksController;
class FireworksEffectObj;
class FishColony;
namespace al { class FixMapParts; }
class FlipCircusDoorA;
class FlingPole;
class FlipPanel;
namespace al { class FloaterMapParts; }
class FloatingTerrain;
class FloatingIslandRailPart;
class FlowerCat;
class FlowerCactus;
class FlowerRhythm;
class FlyOverCamera;
namespace al { class FogRequester; }
class FortressGoal;
class FrameOutChecker;
class Fugumannen;
class Gabon;
class Gamane;
class GateKeeperChecker;
namespace al { class GateMapParts; }
class Gesso;
class GeneratorBox;
class GhostPlayerPlayer;
class GiantTouchBreakMapParts;
class GigaBellItem;
class GoalBonusGameBlockSlot;
class GoalDoor;
class GoalItem;
class GoalItemWatcher;
class GoalPedestal;
class GoalPole;
namespace al { class GodRayRequester; }
class Gondola;
class Gong;
class Gorobon;
class Gotogoton;
class GraphicsAreaController;
namespace al { class GraphicsObjShadowMaskCube; }
namespace al { class GraphicsObjShadowMaskSphere; }
class GrassHigh;
class GreenCoin;
class GreenRing;
class GreenStar;
class GreenStarStand;
class GroupClippingDummyTarget;
class GustWind;
class GuideObj;
class GuideMessageAppear;
class Hacchin;
class HammerBros;
class HexScrollAppearParts;
class IllustItem;
class Imozo;
class InkBomb;
class InkPatch;
class InkPuddle;
class InteractableCatToy;
class IntroFlyOverCamera;
class IslandFlag;
class IslandKeyMoveMapParts;
class ItemBubbleSingleMode;
class JumpFlipPanel;
class JumpFlipSweetsCream;
class JumpPanel;
class Kameck;
class KarakuriCastleDoorWatcher;
class KaronWing;
namespace al { class KeyMoveMapParts; }
namespace al { class KeyMoveMapPartsGenerator; }
class KeyMoveLoopLiftGenerator;
class KillerLauncher;
class KillerTankPartsNeedle;
class KinokoGiga;
class KinokoOneUp;
class KinokoOneUpCourseSelect;
class KinokoSuper;
class KinokoTreasure;
class KinopioBrigadeChecker;
class KinopioBrigadeNpc;
class KinopioBrigadeWatcher;
class KinopioNpc;
class Koura;
class KouraGold;
class KoopaChase;
class KoopaChaseWarpDummy;
class KoopaChaseCar;
class KoopaFireBallGenerator;
class KoopaGraffiti;
class KoopaLastAttackFire;
class KoopaLastBeforeCheckPoint;
class KoopaLastBlockPow;
class KoopaLastBreakWall;
class KoopaLastDefendeBlockPow;
class KoopaLastDemoAppear;
class KoopaLastFloor;
class KoopaLastWall;
class KoopaLastWallClimb;
class KoopaLastWallClimbDown;
class KoopaSignBoard;
class Kuribo;
class KuriboClimbRail;
class KuriboClimbSyncObj;
class KuriboMini;
class KuriboMiniGenerator;
class KuriboTower;
class KyoroHei;
class Kyuppon;
class Lantern;
class LavaGeyser;
class LiftMike;
class LiftMikeBlockRail;
class LiftMikeSlide;
class Lighthouse;
class LighthouseSimple;
class LightningController;
class LuckyIsland;
class LuckyIslandController;
class MagmaFish;
class MarchGenerator;
class MeraWanwan;
class Mirror;
class MoamoaHolder;
class MultiLift;
class MysteryBox;
class MysteryHouseChecker;
class NeedleBarRoot;
class NeedleRoller;
class NeedleRollerGenerator;
class NeedleRollerSwing;
class NeedleSeed;
class NeedleTrap;
class Neko;
class Nokonoko;
namespace al { class OccludedEffectRequester; }
class OceanWaterIndirect;
class OceanWaterDeferred;
class OneSideStep;
class OneSideStepGenerator;
class PackunFire;
class PackunFlower;
class PackunFlowerWithPot;
class PanelNotePaint;
class PanelNotePaintGroup;
class Peto;
class PipePackun;
class PlayerPointLightingObj;
class PlayerSpotLightingObj;
class PlessieTerrain;
namespace al { class PrePassLineLight; }
namespace al { class PrePassPointLight; }
namespace al { class PrePassProjLight; }
namespace al { class PrePassProjOrthoLight; }
namespace al { class PrePassSpotLight; }
class PressureDeathObj;
class PSwitchTimerCoinWatcher;
class BoxPropeller;
class PlayerNpc;
class Prominence;
class Pukupuku;
class PukupukuFly;
class Punpun;
class Rabbit;
class RabbitNpc;
class RaftConveyer;
class Raidon;
class RaidonNpc;
class RaidonSurf;
namespace al { class RailMoveMapParts; }
class Kuribon;
class RingBeamer;
namespace al { class RollingCubeMapParts; }
namespace al { class RollingCubeMapPartsGenerator; }
namespace al { class RotateMapParts; }
class RouteDokan;
class RouteDokanBazooka;
class RouteDokanLauncher;
class Runner;
class SaveDataChecker;
class SamboSnowHead;
class SePlayObj;
namespace al { class SePlayRail; }
class Seaweed;
namespace al { class SeesawMapParts; }
class Shards;
class ShardsWatcher;
class ShadowMarioPlayer;
class SignBoard;
class SingleModeCheckpoint;
class SinkedItem;
class SkateShoes;
class Skatis;
class Skipper;
class SkipperWeak;
namespace al { class Sky; }
class GameSkyProjection;
namespace al { class SlideMapParts; }
class SoundKeyMoveParts;
class Spinner;
class SplatterPlotter;
class SpotLightPatroller;
class SpotLightPatrollerObserver;
class StageStartBindDemoCasinoRoom;
class StageStartBindDemoKinopioBrigade;
class StageStartBindDemoKinopioHouse;
class StageStartBindDemoMysteryHouse;
class StageStartEventCamera;
class StageStartEventDemo;
class StageStartEventSound;
class StepScrollWatcher;
class StrongHipDropReactPoint;
class SuperBell;
class SuperBellSpecial;
class GigaBell;
class KuriboGiga;
class GigaBellManager;
class GigaBellPedestal;
class GigaRock;
class SuperBowser;
class SuperBowserShell;
class SuperbViewArea;
class SuperKoura;
class SuperLeaf;
class SuperSkateRail;
class SuperSkateShoes;
class SuperStar;
namespace al { class SurfMapParts; }
class Swimmer;
namespace al { class SwingMapParts; }
class SwitchAnd;
class SwitchRotateWatcher;
class SwitchBlockWatcher;
class SyumockConveyerGenerator;
class SyumockRailMove;
class SyumockRotate;
class Takobo;
class Tentack;
class TentackLv3;
class TentenGenerator;
class Teren;
class Teresa;
class TeresaConveyorBench;
class TeresaConveyorTeresaBig;
class TeresaFakeDokan;
class TeresaFakeObject;
class TeresaWall;
class Tico;
class TicoCourseSelect;
class TimerClock;
class TimerCoinHolder;
class TimerGate;
class TimerStageSwitch;
class Togezo;
class TouchReactionMapParts;
class TrampleSwitchTimer;
class TrampleSwitch;
class TrampleSwitchChara;
class TrampleSwitchGoalItem;
class TrampleSwitchGold;
class TrampleSwitchTogetherWatcher;
class Trampoline;
class TransparentWall;
class Trapeze;
class Tree;
class TreeFarLodWatcher;
class TreeStump;
class TreeStumpWatcher;
class Tuccondor;
class TuccondorAround;
class TuccondorTrap;
class Ukibo;
class VisibleSwitchMapParts;
class WarpCube;
class WarpDoor;
class WaterAreaMoveModel;
namespace al { class WheelMapParts; }
class WheelWatcher;
namespace al { class WobbleMapParts; }
class WoodBox;
class WoodLogBridge;
class ZigzagBuildingCover;
class PlayerKoopaJr;
class ThrowMapParts;
class TimeLimitStepSwitch;
class WindowMessageAppear;
class IslandHolder;
class TestAndoGoalPole;

extern template al::LiveActor* al::createActorFunction<AllDeadWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<DisasterBlockDeadWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<AssistSlideMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<AssistSlideMapPartsGroup>(const char*);
extern template al::LiveActor* al::createActorFunction<BalanceTruck>(const char*);
extern template al::LiveActor* al::createActorFunction<Ball>(const char*);
extern template al::LiveActor* al::createActorFunction<BallGimmick>(const char*);
extern template al::LiveActor* al::createActorFunction<BallSnow>(const char*);
extern template al::LiveActor* al::createActorFunction<BallYarn>(const char*);
extern template al::LiveActor* al::createActorFunction<BgmPlayObj>(const char*);
extern template al::LiveActor* al::createActorFunction<BgmStopObj>(const char*);
extern template al::LiveActor* al::createActorFunction<BgmRegionChanger>(const char*);
extern template al::LiveActor* al::createActorFunction<Bird>(const char*);
extern template al::LiveActor* al::createActorFunction<BlockAssistLeaf>(const char*);
extern template al::LiveActor* al::createActorFunction<BlockBrick>(const char*);
extern template al::LiveActor* al::createActorFunction<BlockBrickBig>(const char*);
extern template al::LiveActor* al::createActorFunction<BlockBrickBreakable>(const char*);
extern template al::LiveActor* al::createActorFunction<BlockBrickBreakableCourseSelect>(const char*);
extern template al::LiveActor* al::createActorFunction<BlockChoiceWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<BlockEmpty>(const char*);
extern template al::LiveActor* al::createActorFunction<BlockEmptyCourseSelect>(const char*);
extern template al::LiveActor* al::createActorFunction<BlockHard>(const char*);
extern template al::LiveActor* al::createActorFunction<BlockHardLaserOnly>(const char*);
extern template al::LiveActor* al::createActorFunction<BlockPow>(const char*);
extern template al::LiveActor* al::createActorFunction<BlockQuestion>(const char*);
extern template al::LiveActor* al::createActorFunction<BlockQuestionCourseSelect>(const char*);
extern template al::LiveActor* al::createActorFunction<BlockRoulette>(const char*);
extern template al::LiveActor* al::createActorFunction<al::BlockRail>(const char*);
extern template al::LiveActor* al::createActorFunction<BlockRailMover>(const char*);
extern template al::LiveActor* al::createActorFunction<BlockSlot>(const char*);
extern template al::LiveActor* al::createActorFunction<BlockSwitch>(const char*);
extern template al::LiveActor* al::createActorFunction<BlockTransparent>(const char*);
extern template al::LiveActor* al::createActorFunction<BobsledDashPanel>(const char*);
extern template al::LiveActor* al::createActorFunction<Bomb>(const char*);
extern template al::LiveActor* al::createActorFunction<BombBound>(const char*);
extern template al::LiveActor* al::createActorFunction<BombBoundGenerator>(const char*);
extern template al::LiveActor* al::createActorFunction<BombHei>(const char*);
extern template al::LiveActor* al::createActorFunction<BombHeiLauncher>(const char*);
extern template al::LiveActor* al::createActorFunction<Bonze>(const char*);
extern template al::LiveActor* al::createActorFunction<BoomerangBros>(const char*);
extern template al::LiveActor* al::createActorFunction<BoomerangFlower>(const char*);
extern template al::LiveActor* al::createActorFunction<BossBunretsu>(const char*);
extern template al::LiveActor* al::createActorFunction<BossGorobon>(const char*);
extern template al::LiveActor* al::createActorFunction<BossWackun>(const char*);
extern template al::LiveActor* al::createActorFunction<BoxCoin>(const char*);
extern template al::LiveActor* al::createActorFunction<BoxKiller>(const char*);
extern template al::LiveActor* al::createActorFunction<BoxKillerLauncher>(const char*);
extern template al::LiveActor* al::createActorFunction<BoxKuribo>(const char*);
extern template al::LiveActor* al::createActorFunction<BoxLight>(const char*);
extern template al::LiveActor* al::createActorFunction<BreakMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<Bubble>(const char*);
extern template al::LiveActor* al::createActorFunction<Bull>(const char*);
extern template al::LiveActor* al::createActorFunction<Bunbun>(const char*);
extern template al::LiveActor* al::createActorFunction<Bush>(const char*);
extern template al::LiveActor* al::createActorFunction<Byugo>(const char*);
extern template al::LiveActor* al::createActorFunction<CameraInSwitchOnAreaWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<CameraLookAtPoint>(const char*);
extern template al::LiveActor* al::createActorFunction<al::CameraRailHolder_RS>(const char*);
extern template al::LiveActor* al::createActorFunction<CameraRailObserver>(const char*);
extern template al::LiveActor* al::createActorFunction<CameraWall>(const char*);
extern template al::LiveActor* al::createActorFunction<Candlestand>(const char*);
extern template al::LiveActor* al::createActorFunction<CandlestandWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<CatGull>(const char*);
extern template al::LiveActor* al::createActorFunction<CheckPoint>(const char*);
extern template al::LiveActor* al::createActorFunction<CheckpointFlag>(const char*);
extern template al::LiveActor* al::createActorFunction<ChikaChikaBlockWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<ChikuwaBlock>(const char*);
extern template al::LiveActor* al::createActorFunction<ChorobonColony>(const char*);
extern template al::LiveActor* al::createActorFunction<ChorobonCubeHolder>(const char*);
extern template al::LiveActor* al::createActorFunction<ChorobonHolder>(const char*);
extern template al::LiveActor* al::createActorFunction<ClimbHandle>(const char*);
extern template al::LiveActor* al::createActorFunction<ClimbHandleMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<al::ClockMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<CloudBonusLauncher>(const char*);
extern template al::LiveActor* al::createActorFunction<Coin>(const char*);
extern template al::LiveActor* al::createActorFunction<CoinBlowConcentric>(const char*);
extern template al::LiveActor* al::createActorFunction<CoinBlowGenerator>(const char*);
extern template al::LiveActor* al::createActorFunction<CoinChameleon>(const char*);
extern template al::LiveActor* al::createActorFunction<CoinChameleonCourseSelect>(const char*);
extern template al::LiveActor* al::createActorFunction<CoinCirclePlacement>(const char*);
extern template al::LiveActor* al::createActorFunction<CoinCollectWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<CoinConcentricCircle>(const char*);
extern template al::LiveActor* al::createActorFunction<CoinCourseSelect>(const char*);
extern template al::LiveActor* al::createActorFunction<CoinFallGenerator>(const char*);
extern template al::LiveActor* al::createActorFunction<CoinFalls>(const char*);
extern template al::LiveActor* al::createActorFunction<CoinLine>(const char*);
extern template al::LiveActor* al::createActorFunction<CoinRail>(const char*);
extern template al::LiveActor* al::createActorFunction<CoinRedRing>(const char*);
extern template al::LiveActor* al::createActorFunction<CoinRing>(const char*);
extern template al::LiveActor* al::createActorFunction<CoinStack>(const char*);
extern template al::LiveActor* al::createActorFunction<CoinStackMoving>(const char*);
extern template al::LiveActor* al::createActorFunction<CollectRingHolder>(const char*);
extern template al::LiveActor* al::createActorFunction<CollisionSearchObj>(const char*);
extern template al::LiveActor* al::createActorFunction<al::ConveyerMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<CourseSelectBgmEventController>(const char*);
extern template al::LiveActor* al::createActorFunction<CourseSelectCloud>(const char*);
extern template al::LiveActor* al::createActorFunction<CourseSelectDokan>(const char*);
extern template al::LiveActor* al::createActorFunction<CourseSelectMiniature>(const char*);
extern template al::LiveActor* al::createActorFunction<CourseSelectNode>(const char*);
extern template al::LiveActor* al::createActorFunction<CourseSelectRhythmObject>(const char*);
extern template al::LiveActor* al::createActorFunction<CourseSelectRocket>(const char*);
extern template al::LiveActor* al::createActorFunction<CourseSelectRouteDokan>(const char*);
extern template al::LiveActor* al::createActorFunction<CourseSelectWall>(const char*);
extern template al::LiveActor* al::createActorFunction<CourseSelectWorldWarpDokan>(const char*);
extern template al::LiveActor* al::createActorFunction<Crab>(const char*);
extern template al::LiveActor* al::createActorFunction<Crawler>(const char*);
extern template al::LiveActor* al::createActorFunction<CrawlerGenerator>(const char*);
extern template al::LiveActor* al::createActorFunction<CubeMapController>(const char*);
extern template al::LiveActor* al::createActorFunction<DarkBowser>(const char*);
extern template al::LiveActor* al::createActorFunction<DarkBowserLaserIndicator>(const char*);
extern template al::LiveActor* al::createActorFunction<DashPanel>(const char*);
extern template al::LiveActor* al::createActorFunction<DestructableMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<DisasterFixMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<DemoEventGateKeeperChecker>(const char*);
extern template al::LiveActor* al::createActorFunction<DemoAnimatic>(const char*);
extern template al::LiveActor* al::createActorFunction<DemoCutscene>(const char*);
extern template al::LiveActor* al::createActorFunction<DemoKoopaW7>(const char*);
extern template al::LiveActor* al::createActorFunction<DemoObjBase>(const char*);
extern template al::LiveActor* al::createActorFunction<DemoOpeningSwitch>(const char*);
extern template al::LiveActor* al::createActorFunction<DemoStartPosition>(const char*);
extern template al::LiveActor* al::createActorFunction<DemoTimerStageSwitchController>(const char*);
extern template al::LiveActor* al::createActorFunction<DisasterModeController>(const char*);
extern template al::LiveActor* al::createActorFunction<DisasterSpike>(const char*);
extern template al::LiveActor* al::createActorFunction<DisasterSpikeBouncy>(const char*);
extern template al::LiveActor* al::createActorFunction<al::DirectionalLightRequester>(const char*);
extern template al::LiveActor* al::createActorFunction<Dokan>(const char*);
extern template al::LiveActor* al::createActorFunction<DoorKey>(const char*);
extern template al::LiveActor* al::createActorFunction<DoorLock>(const char*);
extern template al::LiveActor* al::createActorFunction<DokanWorldWarp>(const char*);
extern template al::LiveActor* al::createActorFunction<Donketsu>(const char*);
extern template al::LiveActor* al::createActorFunction<Dossun>(const char*);
extern template al::LiveActor* al::createActorFunction<DoubleMario>(const char*);
extern template al::LiveActor* al::createActorFunction<EchoBlockMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<rc::EffectObjGame>(const char*);
extern template al::LiveActor* al::createActorFunction<EffectObjBattleArenaBunBun>(const char*);
extern template al::LiveActor* al::createActorFunction<rc::EffectObjFollowCameraGame>(const char*);
extern template al::LiveActor* al::createActorFunction<EnemyGenerator>(const char*);
extern template al::LiveActor* al::createActorFunction<EnterCatMarioMiddleViewRocket>(const char*);
extern template al::LiveActor* al::createActorFunction<al::FallMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<FairyHouseIllustItemWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<FairyMii>(const char*);
extern template al::LiveActor* al::createActorFunction<FairyNpc>(const char*);
extern template al::LiveActor* al::createActorFunction<FairyNpcWithGlasses>(const char*);
extern template al::LiveActor* al::createActorFunction<FairyPrincess>(const char*);
extern template al::LiveActor* al::createActorFunction<FallingPillar>(const char*);
extern template al::LiveActor* al::createActorFunction<FieryRotateParts>(const char*);
extern template al::LiveActor* al::createActorFunction<FireBarRoot>(const char*);
extern template al::LiveActor* al::createActorFunction<FireBros>(const char*);
extern template al::LiveActor* al::createActorFunction<FireFlower>(const char*);
extern template al::LiveActor* al::createActorFunction<Fireworks>(const char*);
extern template al::LiveActor* al::createActorFunction<FireworksController>(const char*);
extern template al::LiveActor* al::createActorFunction<FireworksEffectObj>(const char*);
extern template al::LiveActor* al::createActorFunction<FishColony>(const char*);
extern template al::LiveActor* al::createActorFunction<al::FixMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<FlipCircusDoorA>(const char*);
extern template al::LiveActor* al::createActorFunction<FlingPole>(const char*);
extern template al::LiveActor* al::createActorFunction<FlipPanel>(const char*);
extern template al::LiveActor* al::createActorFunction<al::FloaterMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<FloatingTerrain>(const char*);
extern template al::LiveActor* al::createActorFunction<FloatingIslandRailPart>(const char*);
extern template al::LiveActor* al::createActorFunction<FlowerCat>(const char*);
extern template al::LiveActor* al::createActorFunction<FlowerCactus>(const char*);
extern template al::LiveActor* al::createActorFunction<FlowerRhythm>(const char*);
extern template al::LiveActor* al::createActorFunction<FlyOverCamera>(const char*);
extern template al::LiveActor* al::createActorFunction<al::FogRequester>(const char*);
extern template al::LiveActor* al::createActorFunction<FortressGoal>(const char*);
extern template al::LiveActor* al::createActorFunction<FrameOutChecker>(const char*);
extern template al::LiveActor* al::createActorFunction<Fugumannen>(const char*);
extern template al::LiveActor* al::createActorFunction<Gabon>(const char*);
extern template al::LiveActor* al::createActorFunction<Gamane>(const char*);
extern template al::LiveActor* al::createActorFunction<GateKeeperChecker>(const char*);
extern template al::LiveActor* al::createActorFunction<al::GateMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<Gesso>(const char*);
extern template al::LiveActor* al::createActorFunction<GeneratorBox>(const char*);
extern template al::LiveActor* al::createActorFunction<GhostPlayerPlayer>(const char*);
extern template al::LiveActor* al::createActorFunction<GiantTouchBreakMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<GigaBellItem>(const char*);
extern template al::LiveActor* al::createActorFunction<GoalBonusGameBlockSlot>(const char*);
extern template al::LiveActor* al::createActorFunction<GoalDoor>(const char*);
extern template al::LiveActor* al::createActorFunction<GoalItem>(const char*);
extern template al::LiveActor* al::createActorFunction<GoalItemWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<GoalPedestal>(const char*);
extern template al::LiveActor* al::createActorFunction<GoalPole>(const char*);
extern template al::LiveActor* al::createActorFunction<al::GodRayRequester>(const char*);
extern template al::LiveActor* al::createActorFunction<Gondola>(const char*);
extern template al::LiveActor* al::createActorFunction<Gong>(const char*);
extern template al::LiveActor* al::createActorFunction<Gorobon>(const char*);
extern template al::LiveActor* al::createActorFunction<Gotogoton>(const char*);
extern template al::LiveActor* al::createActorFunction<GraphicsAreaController>(const char*);
extern template al::LiveActor* al::createActorFunction<al::GraphicsObjShadowMaskCube>(const char*);
extern template al::LiveActor* al::createActorFunction<al::GraphicsObjShadowMaskSphere>(const char*);
extern template al::LiveActor* al::createActorFunction<GrassHigh>(const char*);
extern template al::LiveActor* al::createActorFunction<GreenCoin>(const char*);
extern template al::LiveActor* al::createActorFunction<GreenRing>(const char*);
extern template al::LiveActor* al::createActorFunction<GreenStar>(const char*);
extern template al::LiveActor* al::createActorFunction<GreenStarStand>(const char*);
extern template al::LiveActor* al::createActorFunction<GroupClippingDummyTarget>(const char*);
extern template al::LiveActor* al::createActorFunction<GustWind>(const char*);
extern template al::LiveActor* al::createActorFunction<GuideObj>(const char*);
extern template al::LiveActor* al::createActorFunction<GuideMessageAppear>(const char*);
extern template al::LiveActor* al::createActorFunction<Hacchin>(const char*);
extern template al::LiveActor* al::createActorFunction<HammerBros>(const char*);
extern template al::LiveActor* al::createActorFunction<HexScrollAppearParts>(const char*);
extern template al::LiveActor* al::createActorFunction<IllustItem>(const char*);
extern template al::LiveActor* al::createActorFunction<Imozo>(const char*);
extern template al::LiveActor* al::createActorFunction<InkBomb>(const char*);
extern template al::LiveActor* al::createActorFunction<InkPatch>(const char*);
extern template al::LiveActor* al::createActorFunction<InkPuddle>(const char*);
extern template al::LiveActor* al::createActorFunction<InteractableCatToy>(const char*);
extern template al::LiveActor* al::createActorFunction<IntroFlyOverCamera>(const char*);
extern template al::LiveActor* al::createActorFunction<IslandFlag>(const char*);
extern template al::LiveActor* al::createActorFunction<IslandKeyMoveMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<ItemBubbleSingleMode>(const char*);
extern template al::LiveActor* al::createActorFunction<JumpFlipPanel>(const char*);
extern template al::LiveActor* al::createActorFunction<JumpFlipSweetsCream>(const char*);
extern template al::LiveActor* al::createActorFunction<JumpPanel>(const char*);
extern template al::LiveActor* al::createActorFunction<Kameck>(const char*);
extern template al::LiveActor* al::createActorFunction<KarakuriCastleDoorWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<KaronWing>(const char*);
extern template al::LiveActor* al::createActorFunction<al::KeyMoveMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<al::KeyMoveMapPartsGenerator>(const char*);
extern template al::LiveActor* al::createActorFunction<KeyMoveLoopLiftGenerator>(const char*);
extern template al::LiveActor* al::createActorFunction<KillerLauncher>(const char*);
extern template al::LiveActor* al::createActorFunction<KillerTankPartsNeedle>(const char*);
extern template al::LiveActor* al::createActorFunction<KinokoGiga>(const char*);
extern template al::LiveActor* al::createActorFunction<KinokoOneUp>(const char*);
extern template al::LiveActor* al::createActorFunction<KinokoOneUpCourseSelect>(const char*);
extern template al::LiveActor* al::createActorFunction<KinokoSuper>(const char*);
extern template al::LiveActor* al::createActorFunction<KinokoTreasure>(const char*);
extern template al::LiveActor* al::createActorFunction<KinopioBrigadeChecker>(const char*);
extern template al::LiveActor* al::createActorFunction<KinopioBrigadeNpc>(const char*);
extern template al::LiveActor* al::createActorFunction<KinopioBrigadeWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<KinopioNpc>(const char*);
extern template al::LiveActor* al::createActorFunction<Koura>(const char*);
extern template al::LiveActor* al::createActorFunction<KouraGold>(const char*);
extern template al::LiveActor* al::createActorFunction<KoopaChase>(const char*);
extern template al::LiveActor* al::createActorFunction<KoopaChaseWarpDummy>(const char*);
extern template al::LiveActor* al::createActorFunction<KoopaChaseCar>(const char*);
extern template al::LiveActor* al::createActorFunction<KoopaFireBallGenerator>(const char*);
extern template al::LiveActor* al::createActorFunction<KoopaGraffiti>(const char*);
extern template al::LiveActor* al::createActorFunction<KoopaLastAttackFire>(const char*);
extern template al::LiveActor* al::createActorFunction<KoopaLastBeforeCheckPoint>(const char*);
extern template al::LiveActor* al::createActorFunction<KoopaLastBlockPow>(const char*);
extern template al::LiveActor* al::createActorFunction<KoopaLastBreakWall>(const char*);
extern template al::LiveActor* al::createActorFunction<KoopaLastDefendeBlockPow>(const char*);
extern template al::LiveActor* al::createActorFunction<KoopaLastDemoAppear>(const char*);
extern template al::LiveActor* al::createActorFunction<KoopaLastFloor>(const char*);
extern template al::LiveActor* al::createActorFunction<KoopaLastWall>(const char*);
extern template al::LiveActor* al::createActorFunction<KoopaLastWallClimb>(const char*);
extern template al::LiveActor* al::createActorFunction<KoopaLastWallClimbDown>(const char*);
extern template al::LiveActor* al::createActorFunction<KoopaSignBoard>(const char*);
extern template al::LiveActor* al::createActorFunction<Kuribo>(const char*);
extern template al::LiveActor* al::createActorFunction<KuriboClimbRail>(const char*);
extern template al::LiveActor* al::createActorFunction<KuriboClimbSyncObj>(const char*);
extern template al::LiveActor* al::createActorFunction<KuriboMini>(const char*);
extern template al::LiveActor* al::createActorFunction<KuriboMiniGenerator>(const char*);
extern template al::LiveActor* al::createActorFunction<KuriboTower>(const char*);
extern template al::LiveActor* al::createActorFunction<KyoroHei>(const char*);
extern template al::LiveActor* al::createActorFunction<Kyuppon>(const char*);
extern template al::LiveActor* al::createActorFunction<Lantern>(const char*);
extern template al::LiveActor* al::createActorFunction<LavaGeyser>(const char*);
extern template al::LiveActor* al::createActorFunction<LiftMike>(const char*);
extern template al::LiveActor* al::createActorFunction<LiftMikeBlockRail>(const char*);
extern template al::LiveActor* al::createActorFunction<LiftMikeSlide>(const char*);
extern template al::LiveActor* al::createActorFunction<Lighthouse>(const char*);
extern template al::LiveActor* al::createActorFunction<LighthouseSimple>(const char*);
extern template al::LiveActor* al::createActorFunction<LightningController>(const char*);
extern template al::LiveActor* al::createActorFunction<LuckyIsland>(const char*);
extern template al::LiveActor* al::createActorFunction<LuckyIslandController>(const char*);
extern template al::LiveActor* al::createActorFunction<MagmaFish>(const char*);
extern template al::LiveActor* al::createActorFunction<MarchGenerator>(const char*);
extern template al::LiveActor* al::createActorFunction<MeraWanwan>(const char*);
extern template al::LiveActor* al::createActorFunction<Mirror>(const char*);
extern template al::LiveActor* al::createActorFunction<MoamoaHolder>(const char*);
extern template al::LiveActor* al::createActorFunction<MultiLift>(const char*);
extern template al::LiveActor* al::createActorFunction<MysteryBox>(const char*);
extern template al::LiveActor* al::createActorFunction<MysteryHouseChecker>(const char*);
extern template al::LiveActor* al::createActorFunction<NeedleBarRoot>(const char*);
extern template al::LiveActor* al::createActorFunction<NeedleRoller>(const char*);
extern template al::LiveActor* al::createActorFunction<NeedleRollerGenerator>(const char*);
extern template al::LiveActor* al::createActorFunction<NeedleRollerSwing>(const char*);
extern template al::LiveActor* al::createActorFunction<NeedleSeed>(const char*);
extern template al::LiveActor* al::createActorFunction<NeedleTrap>(const char*);
extern template al::LiveActor* al::createActorFunction<Neko>(const char*);
extern template al::LiveActor* al::createActorFunction<Nokonoko>(const char*);
extern template al::LiveActor* al::createActorFunction<al::OccludedEffectRequester>(const char*);
extern template al::LiveActor* al::createActorFunction<OceanWaterIndirect>(const char*);
extern template al::LiveActor* al::createActorFunction<OceanWaterDeferred>(const char*);
extern template al::LiveActor* al::createActorFunction<OneSideStep>(const char*);
extern template al::LiveActor* al::createActorFunction<OneSideStepGenerator>(const char*);
extern template al::LiveActor* al::createActorFunction<PackunFire>(const char*);
extern template al::LiveActor* al::createActorFunction<PackunFlower>(const char*);
extern template al::LiveActor* al::createActorFunction<PackunFlowerWithPot>(const char*);
extern template al::LiveActor* al::createActorFunction<PanelNotePaint>(const char*);
extern template al::LiveActor* al::createActorFunction<PanelNotePaintGroup>(const char*);
extern template al::LiveActor* al::createActorFunction<Peto>(const char*);
extern template al::LiveActor* al::createActorFunction<PipePackun>(const char*);
extern template al::LiveActor* al::createActorFunction<PlayerPointLightingObj>(const char*);
extern template al::LiveActor* al::createActorFunction<PlayerSpotLightingObj>(const char*);
extern template al::LiveActor* al::createActorFunction<PlessieTerrain>(const char*);
extern template al::LiveActor* al::createActorFunction<al::PrePassLineLight>(const char*);
extern template al::LiveActor* al::createActorFunction<al::PrePassPointLight>(const char*);
extern template al::LiveActor* al::createActorFunction<al::PrePassProjLight>(const char*);
extern template al::LiveActor* al::createActorFunction<al::PrePassProjOrthoLight>(const char*);
extern template al::LiveActor* al::createActorFunction<al::PrePassSpotLight>(const char*);
extern template al::LiveActor* al::createActorFunction<PressureDeathObj>(const char*);
extern template al::LiveActor* al::createActorFunction<PSwitchTimerCoinWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<BoxPropeller>(const char*);
extern template al::LiveActor* al::createActorFunction<PlayerNpc>(const char*);
extern template al::LiveActor* al::createActorFunction<Prominence>(const char*);
extern template al::LiveActor* al::createActorFunction<Pukupuku>(const char*);
extern template al::LiveActor* al::createActorFunction<PukupukuFly>(const char*);
extern template al::LiveActor* al::createActorFunction<Punpun>(const char*);
extern template al::LiveActor* al::createActorFunction<Rabbit>(const char*);
extern template al::LiveActor* al::createActorFunction<RabbitNpc>(const char*);
extern template al::LiveActor* al::createActorFunction<RaftConveyer>(const char*);
extern template al::LiveActor* al::createActorFunction<Raidon>(const char*);
extern template al::LiveActor* al::createActorFunction<RaidonNpc>(const char*);
extern template al::LiveActor* al::createActorFunction<RaidonSurf>(const char*);
extern template al::LiveActor* al::createActorFunction<al::RailMoveMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<Kuribon>(const char*);
extern template al::LiveActor* al::createActorFunction<RingBeamer>(const char*);
extern template al::LiveActor* al::createActorFunction<al::RollingCubeMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<al::RollingCubeMapPartsGenerator>(const char*);
extern template al::LiveActor* al::createActorFunction<al::RotateMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<RouteDokan>(const char*);
extern template al::LiveActor* al::createActorFunction<RouteDokanBazooka>(const char*);
extern template al::LiveActor* al::createActorFunction<RouteDokanLauncher>(const char*);
extern template al::LiveActor* al::createActorFunction<Runner>(const char*);
extern template al::LiveActor* al::createActorFunction<SaveDataChecker>(const char*);
extern template al::LiveActor* al::createActorFunction<SamboSnowHead>(const char*);
extern template al::LiveActor* al::createActorFunction<SePlayObj>(const char*);
extern template al::LiveActor* al::createActorFunction<al::SePlayRail>(const char*);
extern template al::LiveActor* al::createActorFunction<Seaweed>(const char*);
extern template al::LiveActor* al::createActorFunction<al::SeesawMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<Shards>(const char*);
extern template al::LiveActor* al::createActorFunction<ShardsWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<ShadowMarioPlayer>(const char*);
extern template al::LiveActor* al::createActorFunction<SignBoard>(const char*);
extern template al::LiveActor* al::createActorFunction<SingleModeCheckpoint>(const char*);
extern template al::LiveActor* al::createActorFunction<SinkedItem>(const char*);
extern template al::LiveActor* al::createActorFunction<SkateShoes>(const char*);
extern template al::LiveActor* al::createActorFunction<Skatis>(const char*);
extern template al::LiveActor* al::createActorFunction<Skipper>(const char*);
extern template al::LiveActor* al::createActorFunction<SkipperWeak>(const char*);
extern template al::LiveActor* al::createActorFunction<al::Sky>(const char*);
extern template al::LiveActor* al::createActorFunction<GameSkyProjection>(const char*);
extern template al::LiveActor* al::createActorFunction<al::SlideMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<SoundKeyMoveParts>(const char*);
extern template al::LiveActor* al::createActorFunction<Spinner>(const char*);
extern template al::LiveActor* al::createActorFunction<SplatterPlotter>(const char*);
extern template al::LiveActor* al::createActorFunction<SpotLightPatroller>(const char*);
extern template al::LiveActor* al::createActorFunction<SpotLightPatrollerObserver>(const char*);
extern template al::LiveActor* al::createActorFunction<StageStartBindDemoCasinoRoom>(const char*);
extern template al::LiveActor* al::createActorFunction<StageStartBindDemoKinopioBrigade>(const char*);
extern template al::LiveActor* al::createActorFunction<StageStartBindDemoKinopioHouse>(const char*);
extern template al::LiveActor* al::createActorFunction<StageStartBindDemoMysteryHouse>(const char*);
extern template al::LiveActor* al::createActorFunction<StageStartEventCamera>(const char*);
extern template al::LiveActor* al::createActorFunction<StageStartEventDemo>(const char*);
extern template al::LiveActor* al::createActorFunction<StageStartEventSound>(const char*);
extern template al::LiveActor* al::createActorFunction<StepScrollWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<StrongHipDropReactPoint>(const char*);
extern template al::LiveActor* al::createActorFunction<SuperBell>(const char*);
extern template al::LiveActor* al::createActorFunction<SuperBellSpecial>(const char*);
extern template al::LiveActor* al::createActorFunction<GigaBell>(const char*);
extern template al::LiveActor* al::createActorFunction<KuriboGiga>(const char*);
extern template al::LiveActor* al::createActorFunction<GigaBellManager>(const char*);
extern template al::LiveActor* al::createActorFunction<GigaBellPedestal>(const char*);
extern template al::LiveActor* al::createActorFunction<GigaRock>(const char*);
extern template al::LiveActor* al::createActorFunction<SuperBowser>(const char*);
extern template al::LiveActor* al::createActorFunction<SuperBowserShell>(const char*);
extern template al::LiveActor* al::createActorFunction<SuperbViewArea>(const char*);
extern template al::LiveActor* al::createActorFunction<SuperKoura>(const char*);
extern template al::LiveActor* al::createActorFunction<SuperLeaf>(const char*);
extern template al::LiveActor* al::createActorFunction<SuperSkateRail>(const char*);
extern template al::LiveActor* al::createActorFunction<SuperSkateShoes>(const char*);
extern template al::LiveActor* al::createActorFunction<SuperStar>(const char*);
extern template al::LiveActor* al::createActorFunction<al::SurfMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<Swimmer>(const char*);
extern template al::LiveActor* al::createActorFunction<al::SwingMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<SwitchAnd>(const char*);
extern template al::LiveActor* al::createActorFunction<SwitchRotateWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<SwitchBlockWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<SyumockConveyerGenerator>(const char*);
extern template al::LiveActor* al::createActorFunction<SyumockRailMove>(const char*);
extern template al::LiveActor* al::createActorFunction<SyumockRotate>(const char*);
extern template al::LiveActor* al::createActorFunction<Takobo>(const char*);
extern template al::LiveActor* al::createActorFunction<Tentack>(const char*);
extern template al::LiveActor* al::createActorFunction<TentackLv3>(const char*);
extern template al::LiveActor* al::createActorFunction<TentenGenerator>(const char*);
extern template al::LiveActor* al::createActorFunction<Teren>(const char*);
extern template al::LiveActor* al::createActorFunction<Teresa>(const char*);
extern template al::LiveActor* al::createActorFunction<TeresaConveyorBench>(const char*);
extern template al::LiveActor* al::createActorFunction<TeresaConveyorTeresaBig>(const char*);
extern template al::LiveActor* al::createActorFunction<TeresaFakeDokan>(const char*);
extern template al::LiveActor* al::createActorFunction<TeresaFakeObject>(const char*);
extern template al::LiveActor* al::createActorFunction<TeresaWall>(const char*);
extern template al::LiveActor* al::createActorFunction<Tico>(const char*);
extern template al::LiveActor* al::createActorFunction<TicoCourseSelect>(const char*);
extern template al::LiveActor* al::createActorFunction<TimerClock>(const char*);
extern template al::LiveActor* al::createActorFunction<TimerCoinHolder>(const char*);
extern template al::LiveActor* al::createActorFunction<TimerGate>(const char*);
extern template al::LiveActor* al::createActorFunction<TimerStageSwitch>(const char*);
extern template al::LiveActor* al::createActorFunction<Togezo>(const char*);
extern template al::LiveActor* al::createActorFunction<TouchReactionMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<TrampleSwitchTimer>(const char*);
extern template al::LiveActor* al::createActorFunction<TrampleSwitch>(const char*);
extern template al::LiveActor* al::createActorFunction<TrampleSwitchChara>(const char*);
extern template al::LiveActor* al::createActorFunction<TrampleSwitchGoalItem>(const char*);
extern template al::LiveActor* al::createActorFunction<TrampleSwitchGold>(const char*);
extern template al::LiveActor* al::createActorFunction<TrampleSwitchTogetherWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<Trampoline>(const char*);
extern template al::LiveActor* al::createActorFunction<TransparentWall>(const char*);
extern template al::LiveActor* al::createActorFunction<Trapeze>(const char*);
extern template al::LiveActor* al::createActorFunction<Tree>(const char*);
extern template al::LiveActor* al::createActorFunction<TreeFarLodWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<TreeStump>(const char*);
extern template al::LiveActor* al::createActorFunction<TreeStumpWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<Tuccondor>(const char*);
extern template al::LiveActor* al::createActorFunction<TuccondorAround>(const char*);
extern template al::LiveActor* al::createActorFunction<TuccondorTrap>(const char*);
extern template al::LiveActor* al::createActorFunction<Ukibo>(const char*);
extern template al::LiveActor* al::createActorFunction<VisibleSwitchMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<WarpCube>(const char*);
extern template al::LiveActor* al::createActorFunction<WarpDoor>(const char*);
extern template al::LiveActor* al::createActorFunction<WaterAreaMoveModel>(const char*);
extern template al::LiveActor* al::createActorFunction<al::WheelMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<WheelWatcher>(const char*);
extern template al::LiveActor* al::createActorFunction<al::WobbleMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<WoodBox>(const char*);
extern template al::LiveActor* al::createActorFunction<WoodLogBridge>(const char*);
extern template al::LiveActor* al::createActorFunction<ZigzagBuildingCover>(const char*);
extern template al::LiveActor* al::createActorFunction<PlayerKoopaJr>(const char*);
extern template al::LiveActor* al::createActorFunction<ThrowMapParts>(const char*);
extern template al::LiveActor* al::createActorFunction<TimeLimitStepSwitch>(const char*);
extern template al::LiveActor* al::createActorFunction<WindowMessageAppear>(const char*);
extern template al::LiveActor* al::createActorFunction<IslandHolder>(const char*);
extern template al::LiveActor* al::createActorFunction<TestAndoGoalPole>(const char*);

namespace {
const al::NameToCreator<al::CreationFuncPtr> sActorEntries[] = {
    {"AllDeadWatcher", &al::createActorFunction<AllDeadWatcher>},
    {"DisasterBlockDeadWatcher", &al::createActorFunction<DisasterBlockDeadWatcher>},
    {"AssistSlideMapParts", &al::createActorFunction<AssistSlideMapParts>},
    {"AssistSlideMapPartsGroup", &al::createActorFunction<AssistSlideMapPartsGroup>},
    {"BalanceTruck", &al::createActorFunction<BalanceTruck>},
    {"Ball", &al::createActorFunction<Ball>},
    {"BallGimmick", &al::createActorFunction<BallGimmick>},
    {"BallSnow", &al::createActorFunction<BallSnow>},
    {"BallYarn", &al::createActorFunction<BallYarn>},
    {"BallNeko", &al::createActorFunction<BallYarn>},
    {"BgmPlayObj", &al::createActorFunction<BgmPlayObj>},
    {"BgmStopObj", &al::createActorFunction<BgmStopObj>},
    {"BgmRegionChanger", &al::createActorFunction<BgmRegionChanger>},
    {"Bird", &al::createActorFunction<Bird>},
    {"BlockAssistLeaf", &al::createActorFunction<BlockAssistLeaf>},
    {"BlockBrick", &al::createActorFunction<BlockBrick>},
    {"BlockBrickBig", &al::createActorFunction<BlockBrickBig>},
    {"BlockBrickBreakable", &al::createActorFunction<BlockBrickBreakable>},
    {"BlockBrickBreakableCourseSelect", &al::createActorFunction<BlockBrickBreakableCourseSelect>},
    {"BlockChoiceWatcher", &al::createActorFunction<BlockChoiceWatcher>},
    {"BlockEmpty", &al::createActorFunction<BlockEmpty>},
    {"BlockEmptyCourseSelect", &al::createActorFunction<BlockEmptyCourseSelect>},
    {"BlockHard", &al::createActorFunction<BlockHard>},
    {"BlockHardLaserOnly", &al::createActorFunction<BlockHardLaserOnly>},
    {"BlockPow", &al::createActorFunction<BlockPow>},
    {"BlockQuestion", &al::createActorFunction<BlockQuestion>},
    {"BlockQuestionCourseSelect", &al::createActorFunction<BlockQuestionCourseSelect>},
    {"BlockRoulette", &al::createActorFunction<BlockRoulette>},
    {"BlockRail", &al::createActorFunction<al::BlockRail>},
    {"BlockRailMover", &al::createActorFunction<BlockRailMover>},
    {"BlockSlot", &al::createActorFunction<BlockSlot>},
    {"BlockSwitch", &al::createActorFunction<BlockSwitch>},
    {"BlockTransparent", &al::createActorFunction<BlockTransparent>},
    {"BobsledDashPanel", &al::createActorFunction<BobsledDashPanel>},
    {"Bomb", &al::createActorFunction<Bomb>},
    {"BombBound", &al::createActorFunction<BombBound>},
    {"BombBoundGenerator", &al::createActorFunction<BombBoundGenerator>},
    {"BombBoundQuickGenerator", &al::createActorFunction<BombBoundGenerator>},
    {"BombHei", &al::createActorFunction<BombHei>},
    {"BombHeiLauncher", &al::createActorFunction<BombHeiLauncher>},
    {"Bonze", &al::createActorFunction<Bonze>},
    {"BoomerangBros", &al::createActorFunction<BoomerangBros>},
    {"BoomerangFlower", &al::createActorFunction<BoomerangFlower>},
    {"BossBunretsu", &al::createActorFunction<BossBunretsu>},
    {"BossGorobon", &al::createActorFunction<BossGorobon>},
    {"BossWackun", &al::createActorFunction<BossWackun>},
    {"BoxCoin", &al::createActorFunction<BoxCoin>},
    {"BoxKiller", &al::createActorFunction<BoxKiller>},
    {"BoxKillerLauncher", &al::createActorFunction<BoxKillerLauncher>},
    {"BoxKuribo", &al::createActorFunction<BoxKuribo>},
    {"BoxLight", &al::createActorFunction<BoxLight>},
    {"BreakMapParts", &al::createActorFunction<BreakMapParts>},
    {"BreakMapPartsBomb", &al::createActorFunction<BreakMapParts>},
    {"BreakMapPartsSand", &al::createActorFunction<BreakMapParts>},
    {"BreakMapPartsGiant", &al::createActorFunction<BreakMapParts>},
    {"Bubble", &al::createActorFunction<Bubble>},
    {"Bull", &al::createActorFunction<Bull>},
    {"Bunbun", &al::createActorFunction<Bunbun>},
    {"Bush", &al::createActorFunction<Bush>},
    {"Byugo", &al::createActorFunction<Byugo>},
    {"CameraInSwitchOnAreaWatcher", &al::createActorFunction<CameraInSwitchOnAreaWatcher>},
    {"CameraLookAtPoint", &al::createActorFunction<CameraLookAtPoint>},
    {"CameraRailHolder_RS", &al::createActorFunction<al::CameraRailHolder_RS>},
    {"CameraRailObserver", &al::createActorFunction<CameraRailObserver>},
    {"CameraWall", &al::createActorFunction<CameraWall>},
    {"Candlestand", &al::createActorFunction<Candlestand>},
    {"CandlestandWatcher", &al::createActorFunction<CandlestandWatcher>},
    {"CatGull", &al::createActorFunction<CatGull>},
    {"CheckPoint", &al::createActorFunction<CheckPoint>},
    {"CheckpointFlag", &al::createActorFunction<CheckpointFlag>},
    {"ChikaChikaBlockWatcher", &al::createActorFunction<ChikaChikaBlockWatcher>},
    {"ChikuwaBlock", &al::createActorFunction<ChikuwaBlock>},
    {"ChorobonColony", &al::createActorFunction<ChorobonColony>},
    {"ChorobonCubeHolder", &al::createActorFunction<ChorobonCubeHolder>},
    {"ChorobonHolder", &al::createActorFunction<ChorobonHolder>},
    {"ClimbHandle", &al::createActorFunction<ClimbHandle>},
    {"ClimbHandleMapParts", &al::createActorFunction<ClimbHandleMapParts>},
    {"ClockMapParts", &al::createActorFunction<al::ClockMapParts>},
    {"CloudBonusLauncher", &al::createActorFunction<CloudBonusLauncher>},
    {"Coin", &al::createActorFunction<Coin>},
    {"CoinBlowConcentric", &al::createActorFunction<CoinBlowConcentric>},
    {"CoinBlowGenerator", &al::createActorFunction<CoinBlowGenerator>},
    {"CoinChameleon", &al::createActorFunction<CoinChameleon>},
    {"CoinChameleonCourseSelect", &al::createActorFunction<CoinChameleonCourseSelect>},
    {"CoinCirclePlacement", &al::createActorFunction<CoinCirclePlacement>},
    {"CoinCollectWatcher", &al::createActorFunction<CoinCollectWatcher>},
    {"CoinConcentricCircle", &al::createActorFunction<CoinConcentricCircle>},
    {"CoinCourseSelect", &al::createActorFunction<CoinCourseSelect>},
    {"CoinFallGenerator", &al::createActorFunction<CoinFallGenerator>},
    {"CoinFalls", &al::createActorFunction<CoinFalls>},
    {"CoinLine", &al::createActorFunction<CoinLine>},
    {"CoinRail", &al::createActorFunction<CoinRail>},
    {"CoinRedRing", &al::createActorFunction<CoinRedRing>},
    {"CoinRing", &al::createActorFunction<CoinRing>},
    {"CoinStack", &al::createActorFunction<CoinStack>},
    {"CoinStackMoving", &al::createActorFunction<CoinStackMoving>},
    {"CollectRingHolder", &al::createActorFunction<CollectRingHolder>},
    {"CollisionSearchObj", &al::createActorFunction<CollisionSearchObj>},
    {"ConveyerMapParts", &al::createActorFunction<al::ConveyerMapParts>},
    {"ConveyorMapParts", &al::createActorFunction<al::ConveyerMapParts>},
    {"CourseSelectBgmEventController", &al::createActorFunction<CourseSelectBgmEventController>},
    {"CourseSelectCloud", &al::createActorFunction<CourseSelectCloud>},
    {"CourseSelectDokan", &al::createActorFunction<CourseSelectDokan>},
    {"CourseSelectDokanHide", &al::createActorFunction<CourseSelectDokan>},
    {"CourseSelectMiniature", &al::createActorFunction<CourseSelectMiniature>},
    {"CourseSelectNode", &al::createActorFunction<CourseSelectNode>},
    {"CourseSelectRhythmObject", &al::createActorFunction<CourseSelectRhythmObject>},
    {"CourseSelectRocket", &al::createActorFunction<CourseSelectRocket>},
    {"CourseSelectRouteDokan", &al::createActorFunction<CourseSelectRouteDokan>},
    {"CourseSelectWall", &al::createActorFunction<CourseSelectWall>},
    {"CourseSelectWorldWarpDokan", &al::createActorFunction<CourseSelectWorldWarpDokan>},
    {"Crab", &al::createActorFunction<Crab>},
    {"Crawler", &al::createActorFunction<Crawler>},
    {"CrawlerGenerator", &al::createActorFunction<CrawlerGenerator>},
    {"CubeMapController", &al::createActorFunction<CubeMapController>},
    {"DarkBowser", &al::createActorFunction<DarkBowser>},
    {"DarkBowserLaserIndicator", &al::createActorFunction<DarkBowserLaserIndicator>},
    {"DashPanel", &al::createActorFunction<DashPanel>},
    {"DestructableMapParts", &al::createActorFunction<DestructableMapParts>},
    {"DisasterFixMapParts", &al::createActorFunction<DisasterFixMapParts>},
    {"DisasterFixedMapParts", &al::createActorFunction<DisasterFixMapParts>},
    {"DemoEventGateKeeperChecker", &al::createActorFunction<DemoEventGateKeeperChecker>},
    {"DemoAnimatic", &al::createActorFunction<DemoAnimatic>},
    {"DemoCutscene", &al::createActorFunction<DemoCutscene>},
    {"DemoKoopaW7", &al::createActorFunction<DemoKoopaW7>},
    {"DemoObjBase", &al::createActorFunction<DemoObjBase>},
    {"DemoOpeningSwitch", &al::createActorFunction<DemoOpeningSwitch>},
    {"DemoStartPosition", &al::createActorFunction<DemoStartPosition>},
    {"DemoTimerStageSwitchController", &al::createActorFunction<DemoTimerStageSwitchController>},
    {"DisasterModeController", &al::createActorFunction<DisasterModeController>},
    {"DisasterSpike", &al::createActorFunction<DisasterSpike>},
    {"DisasterSpikeBouncy", &al::createActorFunction<DisasterSpikeBouncy>},
    {"DirectionalLightRequester", &al::createActorFunction<al::DirectionalLightRequester>},
    {"Dokan", &al::createActorFunction<Dokan>},
    {"DoorKey", &al::createActorFunction<DoorKey>},
    {"DoorLock", &al::createActorFunction<DoorLock>},
    {"DokanUpsideDown", &al::createActorFunction<Dokan>},
    {"DokanWorldWarp", &al::createActorFunction<DokanWorldWarp>},
    {"Donketsu", &al::createActorFunction<Donketsu>},
    {"Dossun", &al::createActorFunction<Dossun>},
    {"DoubleMario", &al::createActorFunction<DoubleMario>},
    {"EchoBlockMapParts", &al::createActorFunction<EchoBlockMapParts>},
    {"EffectObj", &al::createActorFunction<rc::EffectObjGame>},
    {"EffectObjBattleArenaBunBun", &al::createActorFunction<EffectObjBattleArenaBunBun>},
    {"EffectObjFollowCamera", &al::createActorFunction<rc::EffectObjFollowCameraGame>},
    {"EnemyGenerator", &al::createActorFunction<EnemyGenerator>},
    {"EnterCatMarioMiddleViewRocket", &al::createActorFunction<EnterCatMarioMiddleViewRocket>},
    {"FallMapParts", &al::createActorFunction<al::FallMapParts>},
    {"FairyHouseIllustItemWatcher", &al::createActorFunction<FairyHouseIllustItemWatcher>},
    {"FairyMii", &al::createActorFunction<FairyMii>},
    {"FairyNpc", &al::createActorFunction<FairyNpc>},
    {"FairyNpcWithGlasses", &al::createActorFunction<FairyNpcWithGlasses>},
    {"FairyPrincess", &al::createActorFunction<FairyPrincess>},
    {"FallingPillar", &al::createActorFunction<FallingPillar>},
    {"FieryRotateMapParts", &al::createActorFunction<FieryRotateParts>},
    {"FireBarRoot", &al::createActorFunction<FireBarRoot>},
    {"FireBros", &al::createActorFunction<FireBros>},
    {"FireFlower", &al::createActorFunction<FireFlower>},
    {"Fireworks", &al::createActorFunction<Fireworks>},
    {"FireworksController", &al::createActorFunction<FireworksController>},
    {"FireworksEffectObj", &al::createActorFunction<FireworksEffectObj>},
    {"FishColony", &al::createActorFunction<FishColony>},
    {"FixMapParts", &al::createActorFunction<al::FixMapParts>},
    {"FixedMapParts", &al::createActorFunction<al::FixMapParts>},
    {"FlipCircusDoorA", &al::createActorFunction<FlipCircusDoorA>},
    {"FlingPole", &al::createActorFunction<FlingPole>},
    {"FlipPanel", &al::createActorFunction<FlipPanel>},
    {"FloaterMapParts", &al::createActorFunction<al::FloaterMapParts>},
    {"FloatingTerrain", &al::createActorFunction<FloatingTerrain>},
    {"FloatingIslandRailPart", &al::createActorFunction<FloatingIslandRailPart>},
    {"FlowerCat", &al::createActorFunction<FlowerCat>},
    {"FlowerCactus", &al::createActorFunction<FlowerCactus>},
    {"FlowerRhythm", &al::createActorFunction<FlowerRhythm>},
    {"FlyOverCamera", &al::createActorFunction<FlyOverCamera>},
    {"FogRequester", &al::createActorFunction<al::FogRequester>},
    {"FortressGoal", &al::createActorFunction<FortressGoal>},
    {"FrameOutChecker", &al::createActorFunction<FrameOutChecker>},
    {"Fugumannen", &al::createActorFunction<Fugumannen>},
    {"Gabon", &al::createActorFunction<Gabon>},
    {"Gamane", &al::createActorFunction<Gamane>},
    {"GateKeeperChecker", &al::createActorFunction<GateKeeperChecker>},
    {"GateMapParts", &al::createActorFunction<al::GateMapParts>},
    {"Gesso", &al::createActorFunction<Gesso>},
    {"GeneratorBox", &al::createActorFunction<GeneratorBox>},
    {"GhostPlayerHolder", &al::createActorFunction<GhostPlayerPlayer>},
    {"GiantTouchBreakMapParts", &al::createActorFunction<GiantTouchBreakMapParts>},
    {"GigaBellItem", &al::createActorFunction<GigaBellItem>},
    {"GoalBonusGameBlockSlot", &al::createActorFunction<GoalBonusGameBlockSlot>},
    {"GoalDoor", &al::createActorFunction<GoalDoor>},
    {"GoalItem", &al::createActorFunction<GoalItem>},
    {"GoalItemWatcher", &al::createActorFunction<GoalItemWatcher>},
    {"GoalPedestal", &al::createActorFunction<GoalPedestal>},
    {"GoalPole", &al::createActorFunction<GoalPole>},
    {"GoalPoleRunaway", &al::createActorFunction<GoalPole>},
    {"GodRayRequester", &al::createActorFunction<al::GodRayRequester>},
    {"Gondola", &al::createActorFunction<Gondola>},
    {"Gong", &al::createActorFunction<Gong>},
    {"Gorobon", &al::createActorFunction<Gorobon>},
    {"Gotogoton", &al::createActorFunction<Gotogoton>},
    {"GraphicsAreaController", &al::createActorFunction<GraphicsAreaController>},
    {"GraphicsObjShadowMaskCube", &al::createActorFunction<al::GraphicsObjShadowMaskCube>},
    {"GraphicsObjShadowMaskSphere", &al::createActorFunction<al::GraphicsObjShadowMaskSphere>},
    {"GrassHigh", &al::createActorFunction<GrassHigh>},
    {"GreenCoin", &al::createActorFunction<GreenCoin>},
    {"GreenRing", &al::createActorFunction<GreenRing>},
    {"GreenStar", &al::createActorFunction<GreenStar>},
    {"GreenStarMisteryBoxSeries", &al::createActorFunction<GreenStar>},
    {"GreenStarKinopioBrigade", &al::createActorFunction<GreenStar>},
    {"GreenStarStand", &al::createActorFunction<GreenStarStand>},
    {"GroupClippingDummyTarget", &al::createActorFunction<GroupClippingDummyTarget>},
    {"GustWind", &al::createActorFunction<GustWind>},
    {"GuideObj", &al::createActorFunction<GuideObj>},
    {"GuideMessageAppear", &al::createActorFunction<GuideMessageAppear>},
    {"Hacchin", &al::createActorFunction<Hacchin>},
    {"HammerBros", &al::createActorFunction<HammerBros>},
    {"HexScrollAppearParts", &al::createActorFunction<HexScrollAppearParts>},
    {"IllustItem", &al::createActorFunction<IllustItem>},
    {"Imozo", &al::createActorFunction<Imozo>},
    {"InkBomb", &al::createActorFunction<InkBomb>},
    {"InkPatch", &al::createActorFunction<InkPatch>},
    {"InkPuddle", &al::createActorFunction<InkPuddle>},
    {"InteractableCatToy", &al::createActorFunction<InteractableCatToy>},
    {"IntroFlyOverCamera", &al::createActorFunction<IntroFlyOverCamera>},
    {"IslandFlag", &al::createActorFunction<IslandFlag>},
    {"IslandKeyMoveMapParts", &al::createActorFunction<IslandKeyMoveMapParts>},
    {"ItemBubbleSingleMode", &al::createActorFunction<ItemBubbleSingleMode>},
    {"JumpFlipPanel", &al::createActorFunction<JumpFlipPanel>},
    {"JumpFlipSweetsCream", &al::createActorFunction<JumpFlipSweetsCream>},
    {"JumpPanel", &al::createActorFunction<JumpPanel>},
    {"Kameck", &al::createActorFunction<Kameck>},
    {"KarakuriCastleDoorWatcher", &al::createActorFunction<KarakuriCastleDoorWatcher>},
    {"KaronWing", &al::createActorFunction<KaronWing>},
    {"KeyMoveMapParts", &al::createActorFunction<al::KeyMoveMapParts>},
    {"KeyMoveMapPartsGenerator", &al::createActorFunction<al::KeyMoveMapPartsGenerator>},
    {"KeyMoveLoopLiftGenerator", &al::createActorFunction<KeyMoveLoopLiftGenerator>},
    {"KillerLauncher", &al::createActorFunction<KillerLauncher>},
    {"KillerTankPartsNeedle", &al::createActorFunction<KillerTankPartsNeedle>},
    {"KinokoGiga", &al::createActorFunction<KinokoGiga>},
    {"KinokoOneUp", &al::createActorFunction<KinokoOneUp>},
    {"KinokoOneUpCourseSelect", &al::createActorFunction<KinokoOneUpCourseSelect>},
    {"KinokoSuper", &al::createActorFunction<KinokoSuper>},
    {"KinokoTreasure", &al::createActorFunction<KinokoTreasure>},
    {"KinopioBrigadeChecker", &al::createActorFunction<KinopioBrigadeChecker>},
    {"KinopioBrigadeNpc", &al::createActorFunction<KinopioBrigadeNpc>},
    {"KinopioBrigadeWatcher", &al::createActorFunction<KinopioBrigadeWatcher>},
    {"KinopioNpc", &al::createActorFunction<KinopioNpc>},
    {"Koura", &al::createActorFunction<Koura>},
    {"KouraGold", &al::createActorFunction<KouraGold>},
    {"KoopaChase", &al::createActorFunction<KoopaChase>},
    {"KoopaChaseWarpDummy", &al::createActorFunction<KoopaChaseWarpDummy>},
    {"KoopaChaseCar", &al::createActorFunction<KoopaChaseCar>},
    {"KoopaFireBallGenerator", &al::createActorFunction<KoopaFireBallGenerator>},
    {"KoopaGraffiti", &al::createActorFunction<KoopaGraffiti>},
    {"KoopaLastAttackFire", &al::createActorFunction<KoopaLastAttackFire>},
    {"KoopaLastBeforeCheckPoint", &al::createActorFunction<KoopaLastBeforeCheckPoint>},
    {"KoopaLastBlockPow", &al::createActorFunction<KoopaLastBlockPow>},
    {"KoopaLastBreakWall", &al::createActorFunction<KoopaLastBreakWall>},
    {"KoopaLastDefendeBlockPow", &al::createActorFunction<KoopaLastDefendeBlockPow>},
    {"KoopaLastDemoAppear", &al::createActorFunction<KoopaLastDemoAppear>},
    {"KoopaLastFloor", &al::createActorFunction<KoopaLastFloor>},
    {"KoopaLastWall", &al::createActorFunction<KoopaLastWall>},
    {"KoopaLastWallClimb", &al::createActorFunction<KoopaLastWallClimb>},
    {"KoopaLastWallClimbDown", &al::createActorFunction<KoopaLastWallClimbDown>},
    {"KoopaSignBoard", &al::createActorFunction<KoopaSignBoard>},
    {"Kuribo", &al::createActorFunction<Kuribo>},
    {"KuriboClimbRail", &al::createActorFunction<KuriboClimbRail>},
    {"KuriboClimbSyncObj", &al::createActorFunction<KuriboClimbSyncObj>},
    {"KuriboMini", &al::createActorFunction<KuriboMini>},
    {"KuriboMiniGenerator", &al::createActorFunction<KuriboMiniGenerator>},
    {"KuriboTower", &al::createActorFunction<KuriboTower>},
    {"KyoroHei", &al::createActorFunction<KyoroHei>},
    {"Kyuppon", &al::createActorFunction<Kyuppon>},
    {"Lantern", &al::createActorFunction<Lantern>},
    {"LavaGeyser", &al::createActorFunction<LavaGeyser>},
    {"LiftMike", &al::createActorFunction<LiftMike>},
    {"LiftMikeBlockRail", &al::createActorFunction<LiftMikeBlockRail>},
    {"LiftMikeSlide", &al::createActorFunction<LiftMikeSlide>},
    {"Lighthouse", &al::createActorFunction<Lighthouse>},
    {"LighthouseSimple", &al::createActorFunction<LighthouseSimple>},
    {"LightningController", &al::createActorFunction<LightningController>},
    {"LuckyIsland", &al::createActorFunction<LuckyIsland>},
    {"LuckyIslandController", &al::createActorFunction<LuckyIslandController>},
    {"MagmaFish", &al::createActorFunction<MagmaFish>},
    {"MarchGenerator", &al::createActorFunction<MarchGenerator>},
    {"MeraWanwan", &al::createActorFunction<MeraWanwan>},
    {"Mirror", &al::createActorFunction<Mirror>},
    {"MoamoaHolder", &al::createActorFunction<MoamoaHolder>},
    {"MultiLift", &al::createActorFunction<MultiLift>},
    {"MysteryBox", &al::createActorFunction<MysteryBox>},
    {"MysteryHouseChecker", &al::createActorFunction<MysteryHouseChecker>},
    {"NeedleBarRoot", &al::createActorFunction<NeedleBarRoot>},
    {"NeedleRoller", &al::createActorFunction<NeedleRoller>},
    {"NeedleRollerGenerator", &al::createActorFunction<NeedleRollerGenerator>},
    {"NeedleRollerSwing", &al::createActorFunction<NeedleRollerSwing>},
    {"NeedleSeed", &al::createActorFunction<NeedleSeed>},
    {"NeedleTrap", &al::createActorFunction<NeedleTrap>},
    {"Neko", &al::createActorFunction<Neko>},
    {"NekoParent", &al::createActorFunction<Neko>},
    {"Nokonoko", &al::createActorFunction<Nokonoko>},
    {"OccludedEffectRequester", &al::createActorFunction<al::OccludedEffectRequester>},
    {"OceanWaterIndirect", &al::createActorFunction<OceanWaterIndirect>},
    {"OceanWaterDynamicV1", &al::createActorFunction<OceanWaterDeferred>},
    {"OceanWaterDynamicDeferred", &al::createActorFunction<OceanWaterDeferred>},
    {"OneSideStep", &al::createActorFunction<OneSideStep>},
    {"OneSideStepGenerator", &al::createActorFunction<OneSideStepGenerator>},
    {"PackunFire", &al::createActorFunction<PackunFire>},
    {"PackunFlower", &al::createActorFunction<PackunFlower>},
    {"PackunFlowerWithPot", &al::createActorFunction<PackunFlowerWithPot>},
    {"PanelNotePaint", &al::createActorFunction<PanelNotePaint>},
    {"PanelNotePaintGroup", &al::createActorFunction<PanelNotePaintGroup>},
    {"Peto", &al::createActorFunction<Peto>},
    {"PipePackun", &al::createActorFunction<PipePackun>},
    {"PlayerPointLightingObj", &al::createActorFunction<PlayerPointLightingObj>},
    {"PlayerSpotLightingObj", &al::createActorFunction<PlayerSpotLightingObj>},
    {"PlessieTerrain", &al::createActorFunction<PlessieTerrain>},
    {"PrePassLineLight", &al::createActorFunction<al::PrePassLineLight>},
    {"PrePassPointLight", &al::createActorFunction<al::PrePassPointLight>},
    {"PrePassProjLight", &al::createActorFunction<al::PrePassProjLight>},
    {"PrePassProjOrthoLight", &al::createActorFunction<al::PrePassProjOrthoLight>},
    {"PrePassSpotLight", &al::createActorFunction<al::PrePassSpotLight>},
    {"PressureDeathObj", &al::createActorFunction<PressureDeathObj>},
    {"PSwitchTimerCoinWatcher", &al::createActorFunction<PSwitchTimerCoinWatcher>},
    {"BoxPropeller", &al::createActorFunction<BoxPropeller>},
    {"PlayerNpc", &al::createActorFunction<PlayerNpc>},
    {"Prominence", &al::createActorFunction<Prominence>},
    {"Pukupuku", &al::createActorFunction<Pukupuku>},
    {"PukupukuFly", &al::createActorFunction<PukupukuFly>},
    {"Punpun", &al::createActorFunction<Punpun>},
    {"Rabbit", &al::createActorFunction<Rabbit>},
    {"RabbitNpc", &al::createActorFunction<RabbitNpc>},
    {"RabbitBig", &al::createActorFunction<Rabbit>},
    {"RaftConveyer", &al::createActorFunction<RaftConveyer>},
    {"Raidon", &al::createActorFunction<Raidon>},
    {"RaidonNpc", &al::createActorFunction<RaidonNpc>},
    {"RaidonSurf", &al::createActorFunction<RaidonSurf>},
    {"RailMoveMapParts", &al::createActorFunction<al::RailMoveMapParts>},
    {"Kuribon", &al::createActorFunction<Kuribon>},
    {"RingBeamer", &al::createActorFunction<RingBeamer>},
    {"RollingCubeMapParts", &al::createActorFunction<al::RollingCubeMapParts>},
    {"RollingCubeMapPartsGenerator", &al::createActorFunction<al::RollingCubeMapPartsGenerator>},
    {"RotateMapParts", &al::createActorFunction<al::RotateMapParts>},
    {"RouteDokan", &al::createActorFunction<RouteDokan>},
    {"RouteDokanBazooka", &al::createActorFunction<RouteDokanBazooka>},
    {"RouteDokanLauncher", &al::createActorFunction<RouteDokanLauncher>},
    {"Runner", &al::createActorFunction<Runner>},
    {"SaveDataChecker", &al::createActorFunction<SaveDataChecker>},
    {"SamboSnowHead", &al::createActorFunction<SamboSnowHead>},
    {"SePlayObj", &al::createActorFunction<SePlayObj>},
    {"SePlayRail", &al::createActorFunction<al::SePlayRail>},
    {"Seaweed", &al::createActorFunction<Seaweed>},
    {"SeesawMapParts", &al::createActorFunction<al::SeesawMapParts>},
    {"Shards", &al::createActorFunction<Shards>},
    {"ShardsWatcher", &al::createActorFunction<ShardsWatcher>},
    {"ShadowMarioPlayerHolder", &al::createActorFunction<ShadowMarioPlayer>},
    {"SignBoard", &al::createActorFunction<SignBoard>},
    {"SingleModeCheckpoint", &al::createActorFunction<SingleModeCheckpoint>},
    {"SingleModeCheckpointFlag", &al::createActorFunction<SingleModeCheckpoint>},
    {"SinkedItem", &al::createActorFunction<SinkedItem>},
    {"SkateShoes", &al::createActorFunction<SkateShoes>},
    {"Skatis", &al::createActorFunction<Skatis>},
    {"Skipper", &al::createActorFunction<Skipper>},
    {"SkipperWeak", &al::createActorFunction<SkipperWeak>},
    {"Sky", &al::createActorFunction<al::Sky>},
    {"SkyProjection", &al::createActorFunction<GameSkyProjection>},
    {"SlideMapParts", &al::createActorFunction<al::SlideMapParts>},
    {"SoundKeyMoveParts", &al::createActorFunction<SoundKeyMoveParts>},
    {"Spinner", &al::createActorFunction<Spinner>},
    {"SplatterPlotter", &al::createActorFunction<SplatterPlotter>},
    {"SpotLightPatroller", &al::createActorFunction<SpotLightPatroller>},
    {"SpotLightPatrollerObserver", &al::createActorFunction<SpotLightPatrollerObserver>},
    {"StageStartBindDemoCasinoRoom", &al::createActorFunction<StageStartBindDemoCasinoRoom>},
    {"StageStartBindDemoKinopioBrigade", &al::createActorFunction<StageStartBindDemoKinopioBrigade>},
    {"StageStartBindDemoKinopioHouse", &al::createActorFunction<StageStartBindDemoKinopioHouse>},
    {"StageStartBindDemoMysteryHouse", &al::createActorFunction<StageStartBindDemoMysteryHouse>},
    {"StageStartEventCamera", &al::createActorFunction<StageStartEventCamera>},
    {"StageStartEventDemo", &al::createActorFunction<StageStartEventDemo>},
    {"StageStartEventSound", &al::createActorFunction<StageStartEventSound>},
    {"StepScrollWatcher", &al::createActorFunction<StepScrollWatcher>},
    {"StrongHipDropReactPoint", &al::createActorFunction<StrongHipDropReactPoint>},
    {"SuperBell", &al::createActorFunction<SuperBell>},
    {"SuperBellSpecial", &al::createActorFunction<SuperBellSpecial>},
    {"GigaBell", &al::createActorFunction<GigaBell>},
    {"KuriboGiga", &al::createActorFunction<KuriboGiga>},
    {"GigaBellManager", &al::createActorFunction<GigaBellManager>},
    {"GigaBellPedestal", &al::createActorFunction<GigaBellPedestal>},
    {"GigaRock", &al::createActorFunction<GigaRock>},
    {"SuperBowser", &al::createActorFunction<SuperBowser>},
    {"SuperBowserShell", &al::createActorFunction<SuperBowserShell>},
    {"SuperbViewArea", &al::createActorFunction<SuperbViewArea>},
    {"SuperKoura", &al::createActorFunction<SuperKoura>},
    {"SuperLeaf", &al::createActorFunction<SuperLeaf>},
    {"SuperSkateRail", &al::createActorFunction<SuperSkateRail>},
    {"SuperSkateShoes", &al::createActorFunction<SuperSkateShoes>},
    {"SuperStar", &al::createActorFunction<SuperStar>},
    {"SurfMapParts", &al::createActorFunction<al::SurfMapParts>},
    {"Swimmer", &al::createActorFunction<Swimmer>},
    {"SwingMapParts", &al::createActorFunction<al::SwingMapParts>},
    {"SwitchAnd", &al::createActorFunction<SwitchAnd>},
    {"SwitchRotateWatcher", &al::createActorFunction<SwitchRotateWatcher>},
    {"SwitchBlockWatcher", &al::createActorFunction<SwitchBlockWatcher>},
    {"SwitchBlockMapParts", &al::createActorFunction<SwitchBlockWatcher>},
    {"SyumockConveyerGenerator", &al::createActorFunction<SyumockConveyerGenerator>},
    {"SyumockRailMove", &al::createActorFunction<SyumockRailMove>},
    {"SyumockRotate", &al::createActorFunction<SyumockRotate>},
    {"Takobo", &al::createActorFunction<Takobo>},
    {"Tentack", &al::createActorFunction<Tentack>},
    {"TentackLv3", &al::createActorFunction<TentackLv3>},
    {"TentenGenerator", &al::createActorFunction<TentenGenerator>},
    {"TentenGeneratorNormal", &al::createActorFunction<TentenGenerator>},
    {"Teren", &al::createActorFunction<Teren>},
    {"Teresa", &al::createActorFunction<Teresa>},
    {"TeresaConveyorBench", &al::createActorFunction<TeresaConveyorBench>},
    {"TeresaConveyorTeresaBig", &al::createActorFunction<TeresaConveyorTeresaBig>},
    {"TeresaFakeDokan", &al::createActorFunction<TeresaFakeDokan>},
    {"TeresaFakeObject", &al::createActorFunction<TeresaFakeObject>},
    {"TeresaWall", &al::createActorFunction<TeresaWall>},
    {"Tico", &al::createActorFunction<Tico>},
    {"TicoCourseSelect", &al::createActorFunction<TicoCourseSelect>},
    {"TimerClock", &al::createActorFunction<TimerClock>},
    {"TimerCoinHolder", &al::createActorFunction<TimerCoinHolder>},
    {"TimerGate", &al::createActorFunction<TimerGate>},
    {"TimerStageSwitch", &al::createActorFunction<TimerStageSwitch>},
    {"Togezo", &al::createActorFunction<Togezo>},
    {"TouchReactionMapParts", &al::createActorFunction<TouchReactionMapParts>},
    {"TrampleSwitchTimer", &al::createActorFunction<TrampleSwitchTimer>},
    {"TrampleSwitch", &al::createActorFunction<TrampleSwitch>},
    {"TrampleSwitchChara", &al::createActorFunction<TrampleSwitchChara>},
    {"TrampleSwitchGoalItem", &al::createActorFunction<TrampleSwitchGoalItem>},
    {"TrampleSwitchGold", &al::createActorFunction<TrampleSwitchGold>},
    {"TrampleSwitchTogetherWatcher", &al::createActorFunction<TrampleSwitchTogetherWatcher>},
    {"Trampoline", &al::createActorFunction<Trampoline>},
    {"TransparentWall", &al::createActorFunction<TransparentWall>},
    {"Trapeze", &al::createActorFunction<Trapeze>},
    {"Tree", &al::createActorFunction<Tree>},
    {"TreeFarLodWatcher", &al::createActorFunction<TreeFarLodWatcher>},
    {"TreeStump", &al::createActorFunction<TreeStump>},
    {"TreeStumpWatcher", &al::createActorFunction<TreeStumpWatcher>},
    {"Tuccondor", &al::createActorFunction<Tuccondor>},
    {"TuccondorAround", &al::createActorFunction<TuccondorAround>},
    {"TuccondorTrap", &al::createActorFunction<TuccondorTrap>},
    {"Ukibo", &al::createActorFunction<Ukibo>},
    {"VisibleSwitchMapParts", &al::createActorFunction<VisibleSwitchMapParts>},
    {"WarpCube", &al::createActorFunction<WarpCube>},
    {"WarpDoor", &al::createActorFunction<WarpDoor>},
    {"WaterAreaMoveModel", &al::createActorFunction<WaterAreaMoveModel>},
    {"WheelMapParts", &al::createActorFunction<al::WheelMapParts>},
    {"WheelWatcher", &al::createActorFunction<WheelWatcher>},
    {"WobbleMapParts", &al::createActorFunction<al::WobbleMapParts>},
    {"WoodBox", &al::createActorFunction<WoodBox>},
    {"WoodLogBridge", &al::createActorFunction<WoodLogBridge>},
    {"ZigzagBuildingCover", &al::createActorFunction<ZigzagBuildingCover>},
    {"TestAndoBowserJr", &al::createActorFunction<PlayerKoopaJr>},
    {"KoopaJr", &al::createActorFunction<PlayerKoopaJr>},
    {"ThrowMapParts", &al::createActorFunction<ThrowMapParts>},
    {"TimeLimitStepSwitch", &al::createActorFunction<TimeLimitStepSwitch>},
    {"WindowMessageAppear", &al::createActorFunction<WindowMessageAppear>},
    {"ZoneHolder", &al::createActorFunction<IslandHolder>},
    {"Island", &al::createActorFunction<IslandHolder>},
    {"TestAndoGoalPole", &al::createActorFunction<TestAndoGoalPole>},
};
}  // namespace

ProjectActorFactory::ProjectActorFactory() : al::ActorFactory("アクターファクトリー") {
    initFactory(sActorEntries);
}
