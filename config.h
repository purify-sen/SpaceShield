#ifndef CONFIG_H
#define CONFIG_H

#include <SDL2/SDL.h>
#include <string>

// --- Cài đặt màn hình ---
constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 600;
const std::string WINDOW_TITLE = "Space Shield";

// --- Đường dẫn tài nguyên ---
const std::string FONT_PATH = "fonts/OpenSans-Regular.ttf";
const std::string PLAYER_DATA_DIR = "playerdata";
const std::string PLAYER_DATA_FILE = PLAYER_DATA_DIR + "/playerdata";
const std::string IMAGE_DIR = "images";
const std::string SOUND_DIR = "sounds"; // Thư mục âm thanh

// Hình ảnh
const std::string IMG_SPACESHIP = IMAGE_DIR + "/mspaceship.png";
const std::string IMG_MISSILE = IMAGE_DIR + "/missile.png";
const std::string IMG_FAST_MISSILE = IMAGE_DIR + "/fmissile.png";
const std::string IMG_WARNING = IMAGE_DIR + "/fwarning.png";
const std::string IMG_SPACE_SHARK = IMAGE_DIR + "/spaceshark.png";
const std::string IMG_SHARK_BULLET = IMAGE_DIR + "/sharkbullet.png";
const std::string IMG_PAUSE_BUTTON = IMAGE_DIR + "/pausebutton.png";

// --- Âm thanh (Chỉ những file yêu cầu) ---
// Hiệu ứng (SFX)
const std::string SFX_SHIELD_HIT = SOUND_DIR + "/shield_hit.wav";
const std::string SFX_PLAYER_HIT = SOUND_DIR + "/player_hit.wav";
// const std::string SFX_EXPLOSION = SOUND_DIR + "/explosion.wav"; // *** Đã loại bỏ ***
const std::string SFX_BUTTON_CLICK = SOUND_DIR + "/button_click.wav";
const std::string SFX_GAME_OVER = SOUND_DIR + "/game_over.wav";
const std::string SFX_WARNING = SOUND_DIR + "/warning.wav";
// Nhạc nền (BGM)
const std::string BGM_MENU = SOUND_DIR + "/menu_music.ogg";
const std::string BGM_GAME = SOUND_DIR + "/game_music.ogg";


// --- Cài đặt âm thanh Mixer ---
constexpr int AUDIO_FREQUENCY = 44100;
constexpr int AUDIO_CHANNELS = 2;
constexpr int AUDIO_CHUNK_SIZE = 2048;
constexpr int DEFAULT_VOLUME = 100; // Âm lượng mặc định (0-100)
constexpr int CHANNEL_SFX = -1; // Phát trên kênh khả dụng đầu tiên
constexpr int CHANNEL_WARNING = 1; // Dành riêng kênh cho cảnh báo

// --- Cài đặt Font ---
constexpr int FONT_SIZE_NORMAL = 24;
constexpr int FONT_SIZE_LARGE = 36;
constexpr int FONT_SIZE_XLARGE = 48;
const SDL_Color TEXT_COLOR = {255, 255, 255, 255}; // Trắng
const SDL_Color BUTTON_COLOR = {100, 100, 100, 255}; // Xám
const SDL_Color SLIDER_BG_COLOR = {255, 255, 255, 255}; // Trắng
const SDL_Color SLIDER_KNOB_COLOR = {255, 0, 0, 255}; // Đỏ
const SDL_Color SLIDER_KNOB_DRAG_COLOR = {255, 255, 0, 255}; // Vàng (khi kéo)
const SDL_Color PAUSE_OVERLAY_COLOR = {0, 0, 0, 128}; // Đen trong suốt

// --- Cài đặt Game ---
constexpr float PI = 3.14159265358979323846f;
constexpr int PLAYER_LIVES = 5;
constexpr int LIFE_ICON_RADIUS = 10;
constexpr int LIFE_ICON_SPACING = 30;
constexpr int LIFE_ICON_START_X = 20;
constexpr int LIFE_ICON_START_Y = 20;
const SDL_Color LIFE_ICON_ACTIVE_COLOR = {0, 0, 255, 255}; // Xanh dương
const SDL_Color LIFE_ICON_INACTIVE_COLOR = {255, 0, 0, 255}; // Đỏ

// --- Tàu người chơi (Chitbox) & Khiên (Arc) ---
const SDL_Rect PLAYER_CHITBOX = {375, 250, 50, 100};
const SDL_Point TRAJECTORY_CENTER = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};
constexpr int TRAJECTORY_RADIUS = 60;
constexpr float SHIELD_ARC_ANGLE = 2.0f * PI / 3.0f;
constexpr float INITIAL_SHIELD_START_ANGLE = -PI / 10.3f;
constexpr float SHIELD_ROTATION_SPEED_FACTOR = 2.0f * PI;
constexpr float DEFAULT_SENSITIVITY = 50.0f;
constexpr float MIN_SENSITIVITY_MULTIPLIER = 0.75f;
constexpr float MAX_SENSITIVITY_MULTIPLIER = 1.25f;
const SDL_Color SHIELD_ARC_COLOR = {128, 0, 128, 255};
const SDL_Color TRAJECTORY_CIRCLE_COLOR = {0, 0, 200, 255};

// --- Kẻ địch: Tên lửa thường (Target) ---
constexpr float DEFAULT_MISSILE_SPEED = 100.0f;
constexpr float MAX_MISSILE_SPEED_RANDOM_FACTOR = 1.0f;
constexpr int MISSILE_WIDTH = 45;
constexpr int MISSILE_HEIGHT = 30;
constexpr SDL_Point MISSILE_CENTER = {15, 30};
constexpr float MISSILE_COLLISION_RADIUS_SQ = 25.0f;

// --- Kẻ địch: Tên lửa nhanh (Fast Missile) ---
constexpr float FAST_MISSILE_SPEED_MULTIPLIER = 4.5f;
constexpr int FAST_MISSILE_WIDTH = 45;
constexpr int FAST_MISSILE_HEIGHT = 30;
constexpr SDL_Point FAST_MISSILE_CENTER = {15, 30};
constexpr float FAST_MISSILE_COLLISION_RADIUS_SQ = 25.0f;
constexpr Uint32 FAST_MISSILE_WARNING_DURATION = 2000;
constexpr int WARNING_ICON_WIDTH = 30;
constexpr int WARNING_ICON_HEIGHT = 45;
constexpr int WARNING_ICON_OFFSET_X = 15;
constexpr int WARNING_ICON_OFFSET_Y = 22;
constexpr float WARNING_ALPHA_MIN = 152.5f;
constexpr float WARNING_ALPHA_RANGE = 102.5f;
constexpr float WARNING_ALPHA_FREQ = 2.0f * PI * 2.0f / 1000.0f;

// --- Kẻ địch: Space Shark ---
constexpr float SHARK_INITIAL_RADIUS = 300.0f;
constexpr float SHARK_MIN_RADIUS = 60.0f;
constexpr float SHARK_ANGULAR_SPEED = 2.0f;
constexpr float SHARK_SPIRAL_SPEED = -20.0f;
constexpr Uint32 SHARK_LIFETIME = 15000;
constexpr Uint32 SHARK_BULLET_INTERVAL = 5000;
constexpr int SHARK_WIDTH = 50;
constexpr int SHARK_HEIGHT = 30;
constexpr SDL_Point SHARK_CENTER = {25, 15};
constexpr float SHARK_COLLISION_RADIUS_SQ = 625.0f;

// --- Kẻ địch: Shark Bullet ---
constexpr float SHARK_BULLET_SPEED_MULTIPLIER = 0.5f;
constexpr int SHARK_BULLET_WIDTH = 20;
constexpr int SHARK_BULLET_HEIGHT = 10;
constexpr SDL_Point SHARK_BULLET_CENTER = {10, 5};
constexpr float SHARK_BULLET_COLLISION_RADIUS_SQ = 100.0f;

// --- Gameplay & Waves ---
constexpr int INITIAL_MISSILE_COUNT = 1;
constexpr int MAX_MISSILE_COUNT = 5;
constexpr int BASE_WAVES_UNTIL_INCREASE = 7;
constexpr int RANDOM_WAVES_UNTIL_INCREASE = 6;
constexpr Uint32 INITIAL_SPAWN_DELAY = 2000;
constexpr Uint32 BASE_WAVE_DELAY = 3000;
constexpr Uint32 RANDOM_WAVE_DELAY = 2001;
constexpr Uint32 MISSILE_SPAWN_INTERVAL = 300;
constexpr int SCORE_PER_MISSILE = 1;
constexpr int SCORE_PER_FAST_MISSILE = 2;
constexpr int SCORE_PER_SHARK = 5;
constexpr int WAVE_START_FAST_MISSILE = 9;
constexpr int WAVE_INTERVAL_FAST_MISSILE = 3;
constexpr int WAVE_START_SHARK = 15;
constexpr int WAVE_INTERVAL_SHARK = 15;

// --- Giao diện người dùng (UI) ---
const SDL_Rect PAUSE_BUTTON_RECT = {SCREEN_WIDTH - 50, 10, 40, 40};
constexpr int BUTTON_WIDTH = 200;
constexpr int BUTTON_HEIGHT = 50;
constexpr int GAMEOVER_TITLE_Y = 100;
constexpr int PAUSED_TITLE_Y = 200;
constexpr int SCORE_LABEL_Y = 260;
constexpr int HIGHSCORE_LABEL_Y = 300;
const SDL_Rect RESTART_BUTTON_RECT = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 340, BUTTON_WIDTH, BUTTON_HEIGHT };
const SDL_Rect BACK_TO_MENU_BUTTON_RECT_GAMEOVER = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 400, BUTTON_WIDTH, BUTTON_HEIGHT };
const SDL_Rect GIVE_UP_BUTTON_RECT = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 460, BUTTON_WIDTH, BUTTON_HEIGHT };
constexpr int VOLUME_LABEL_Y = 390;
const SDL_Rect VOLUME_SLIDER_RECT = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 420, BUTTON_WIDTH, 10 };
const SDL_Rect VOLUME_KNOB_RECT = { VOLUME_SLIDER_RECT.x + (DEFAULT_VOLUME * VOLUME_SLIDER_RECT.w / 100) - 5, 415, 10, 20 };
const SDL_Rect PLAY_BUTTON_RECT = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 250, BUTTON_WIDTH, BUTTON_HEIGHT };
const SDL_Rect HIGHSCORE_BUTTON_RECT = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 320, BUTTON_WIDTH, BUTTON_HEIGHT };
const SDL_Rect SETTINGS_BUTTON_RECT = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 390, BUTTON_WIDTH, BUTTON_HEIGHT };
const SDL_Rect EXIT_BUTTON_RECT = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 460, BUTTON_WIDTH, BUTTON_HEIGHT };
const SDL_Rect BACK_BUTTON_RECT = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 500, BUTTON_WIDTH, BUTTON_HEIGHT };
constexpr int HIGHSCORE_TITLE_Y_MENU = 100;
constexpr int HIGHSCORE_LIST_Y = 200;
constexpr int MAX_HIGHSCORES_DISPLAY = 5;
constexpr int SETTINGS_TITLE_Y = 100;
const SDL_Rect VOLUME_SLIDER_RECT_SETTINGS = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 280, BUTTON_WIDTH, 10 };
const SDL_Rect VOLUME_KNOB_RECT_SETTINGS = { VOLUME_SLIDER_RECT_SETTINGS.x + (DEFAULT_VOLUME * VOLUME_SLIDER_RECT_SETTINGS.w / 100) - 5, 275, 10, 20 };
const int VOLUME_LABEL_Y_SETTINGS = VOLUME_SLIDER_RECT_SETTINGS.y - 40;
const SDL_Rect SENSITIVITY_SLIDER_RECT_SETTINGS = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 380, BUTTON_WIDTH, 10 };
const SDL_Rect SENSITIVITY_KNOB_RECT_SETTINGS = { SENSITIVITY_SLIDER_RECT_SETTINGS.x + (int)(DEFAULT_SENSITIVITY * SENSITIVITY_SLIDER_RECT_SETTINGS.w / 100.0f) - 5, 375, 10, 20 };
const int SENSITIVITY_LABEL_Y_SETTINGS = SENSITIVITY_SLIDER_RECT_SETTINGS.y - 40;

// --- Khác ---
constexpr int CIRCLE_SEGMENTS = 36;
constexpr int ARC_SEGMENTS = 24;

#endif // CONFIG_H
