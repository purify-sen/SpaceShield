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
const std::string SOUND_DIR = "sounds";

// Hình ảnh
const std::string IMG_SPACESHIP = IMAGE_DIR + "/mspaceship.png";
const std::string IMG_MISSILE = IMAGE_DIR + "/missile.png";
const std::string IMG_FAST_MISSILE = IMAGE_DIR + "/fmissile.png";
const std::string IMG_WARNING = IMAGE_DIR + "/fwarning.png";
const std::string IMG_SPACE_SHARK = IMAGE_DIR + "/spaceshark.png";
const std::string IMG_SHARK_BULLET = IMAGE_DIR + "/sharkbullet.png";
const std::string IMG_PAUSE_BUTTON = IMAGE_DIR + "/pausebutton.png";
const std::string IMG_MAIN_MENU_BG = IMAGE_DIR + "/mainmenubg.png"; // Ảnh nền Menu
const std::string IMG_GAME_BG = IMAGE_DIR + "/gamebg.png";         // Ảnh nền Game
const std::string IMG_ALLY_SHIP = IMAGE_DIR + "/spacesen.png"; // Đường dẫn ảnh đồng minh
const std::string IMG_HEAL_ITEM = IMAGE_DIR + "/heal.png";     // Đường dẫn ảnh hồi máu

// --- Âm thanh ---
const std::string SFX_SHIELD_HIT = SOUND_DIR + "/shield_hit.wav";
const std::string SFX_PLAYER_HIT = SOUND_DIR + "/player_hit.wav";
const std::string SFX_BUTTON_CLICK = SOUND_DIR + "/button_click.wav";
const std::string SFX_GAME_OVER = SOUND_DIR + "/game_over.wav";
const std::string SFX_WARNING = SOUND_DIR + "/warning.wav";
const std::string SFX_HEAL_COLLECT = SOUND_DIR + "/heal.wav";    // Đường dẫn âm thanh hồi máu
const std::string BGM_MENU = SOUND_DIR + "/menu_music.ogg";
const std::string BGM_GAME = SOUND_DIR + "/game_music.ogg";

// --- Cài đặt âm thanh Mixer ---
constexpr int AUDIO_FREQUENCY = 44100;
constexpr int AUDIO_CHANNELS = 2;
constexpr int AUDIO_CHUNK_SIZE = 2048;
constexpr int DEFAULT_VOLUME = 100;
constexpr int CHANNEL_SFX = -1; // Sử dụng kênh mặc định cho hầu hết SFX
constexpr int CHANNEL_WARNING = 1; // Kênh riêng cho warning để có thể dừng

// --- Cài đặt Font ---
constexpr int FONT_SIZE_SMALL = 18; // <-- THÊM CỠ CHỮ NHỎ MỚI -->
constexpr int FONT_SIZE_NORMAL = 24;
constexpr int FONT_SIZE_LARGE = 36;
constexpr int FONT_SIZE_XLARGE = 48;
const SDL_Color TEXT_COLOR = {255, 255, 255, 255}; // Trắng
const SDL_Color BUTTON_COLOR = {100, 100, 100, 255}; // Xám
const SDL_Color SLIDER_BG_COLOR = {255, 255, 255, 255}; // Trắng
const SDL_Color SLIDER_KNOB_COLOR = {255, 0, 0, 255}; // Đỏ
const SDL_Color SLIDER_KNOB_DRAG_COLOR = {255, 255, 0, 255}; // Vàng
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

// --- Tàu người chơi & Khiên ---
const SDL_Rect PLAYER_CHITBOX = {375, 250, 50, 100}; // Hitbox của tàu người chơi
const SDL_Point TRAJECTORY_CENTER = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2}; // Tâm quỹ đạo
constexpr int TRAJECTORY_RADIUS = 60; // Bán kính quỹ đạo khiên
constexpr float SHIELD_ARC_ANGLE = 2.0f * PI / 3.0f; // Góc của khiên (radian)
constexpr float INITIAL_SHIELD_START_ANGLE = -PI / 10.3f; // Góc bắt đầu ban đầu của khiên
constexpr float SHIELD_ROTATION_SPEED_FACTOR = 2.0f * PI; // Tốc độ xoay cơ bản của khiên
constexpr float DEFAULT_SENSITIVITY = 50.0f; // Độ nhạy mặc định (0-100)
constexpr float MIN_SENSITIVITY_MULTIPLIER = 0.75f; // Hệ số tốc độ xoay ở độ nhạy min
constexpr float MAX_SENSITIVITY_MULTIPLIER = 1.25f; // Hệ số tốc độ xoay ở độ nhạy max
const SDL_Color SHIELD_ARC_COLOR = {128, 0, 128, 255}; // Màu tím cho khiên
const SDL_Color TRAJECTORY_CIRCLE_COLOR = {0, 0, 200, 255}; // Màu xanh dương đậm cho quỹ đạo

// --- Kẻ địch: Tên lửa thường (Target) ---
constexpr float DEFAULT_MISSILE_SPEED = 100.0f; // Tốc độ cơ bản (pixel/giây)
constexpr float MAX_MISSILE_SPEED_RANDOM_FACTOR = 1.0f; // Yếu tố ngẫu nhiên tốc độ (0.0 đến 1.0)
constexpr int MISSILE_WIDTH = 45; // Chiều rộng ảnh tên lửa
constexpr int MISSILE_HEIGHT = 30; // Chiều cao ảnh tên lửa
constexpr SDL_Point MISSILE_CENTER = {15, 30}; // Tâm xoay của ảnh tên lửa (điều chỉnh nếu cần)
constexpr float MISSILE_COLLISION_RADIUS_SQ = 25.0f; // Bình phương bán kính va chạm (đơn giản hóa)

// --- Kẻ địch: Tên lửa nhanh (Fast Missile) ---
constexpr float FAST_MISSILE_SPEED_MULTIPLIER = 4.5f; // Hệ số nhân tốc độ so với tên lửa thường
constexpr int FAST_MISSILE_WIDTH = 45; // Chiều rộng ảnh
constexpr int FAST_MISSILE_HEIGHT = 30; // Chiều cao ảnh
constexpr SDL_Point FAST_MISSILE_CENTER = {15, 30}; // Tâm xoay
constexpr float FAST_MISSILE_COLLISION_RADIUS_SQ = 25.0f; // Bình phương bán kính va chạm
constexpr Uint32 FAST_MISSILE_WARNING_DURATION = 2000; // Thời gian cảnh báo (ms)
constexpr int WARNING_ICON_WIDTH = 30; // Chiều rộng icon cảnh báo
constexpr int WARNING_ICON_HEIGHT = 45; // Chiều cao icon cảnh báo
constexpr int WARNING_ICON_OFFSET_X = 15; // Độ lệch X để căn giữa icon cảnh báo
constexpr int WARNING_ICON_OFFSET_Y = 22; // Độ lệch Y để căn giữa icon cảnh báo
constexpr float WARNING_ALPHA_MIN = 152.5f; // Độ trong suốt tối thiểu (0-255)
constexpr float WARNING_ALPHA_RANGE = 102.5f; // Biên độ thay đổi độ trong suốt
constexpr float WARNING_ALPHA_FREQ = 2.0f * PI * 2.0f / 1000.0f; // Tần số nhấp nháy (radian/ms)

// --- Kẻ địch: Space Shark ---
constexpr float SHARK_INITIAL_RADIUS = 300.0f; // Bán kính xuất hiện ban đầu
constexpr float SHARK_MIN_RADIUS = 60.0f; // Bán kính tối thiểu khi tiến vào
constexpr float SHARK_ANGULAR_SPEED = 2.0f; // Tốc độ góc (rad/giây)
constexpr float SHARK_SPIRAL_SPEED = -20.0f; // Tốc độ đi vào tâm (pixel/giây, âm là đi vào)
constexpr Uint32 SHARK_LIFETIME = 15000; // Thời gian tồn tại tối đa (ms)
constexpr Uint32 SHARK_BULLET_INTERVAL = 5000; // Khoảng thời gian bắn đạn (ms)
constexpr int SHARK_WIDTH = 50; // Chiều rộng ảnh
constexpr int SHARK_HEIGHT = 30; // Chiều cao ảnh
constexpr SDL_Point SHARK_CENTER = {25, 15}; // Tâm xoay
constexpr float SHARK_COLLISION_RADIUS_SQ = 625.0f; // Bình phương bán kính va chạm

// --- Kẻ địch: Shark Bullet ---
constexpr float SHARK_BULLET_SPEED_MULTIPLIER = 0.5f; // Hệ số nhân tốc độ so với tên lửa thường
constexpr int SHARK_BULLET_WIDTH = 20; // Chiều rộng ảnh
constexpr int SHARK_BULLET_HEIGHT = 10; // Chiều cao ảnh
constexpr SDL_Point SHARK_BULLET_CENTER = {10, 5}; // Tâm xoay
constexpr float SHARK_BULLET_COLLISION_RADIUS_SQ = 100.0f; // Bình phương bán kính va chạm

// --- Đồng minh & Hồi máu ---
constexpr Uint32 ALLY_SPAWN_INTERVAL = 15000; // Khoảng thời gian xuất hiện đồng minh (15 giây)
constexpr float ALLY_SPEED = 150.0f; // Tốc độ bay ngang của đồng minh (pixel/giây)
constexpr int ALLY_WIDTH = 50; // Chiều rộng ảnh spacesen.png (điều chỉnh nếu cần)
constexpr int ALLY_HEIGHT = 40; // Chiều cao ảnh spacesen.png (điều chỉnh nếu cần)
constexpr int HEAL_ITEM_WIDTH = 30; // Chiều rộng ảnh heal.png (điều chỉnh nếu cần)
constexpr int HEAL_ITEM_HEIGHT = 30; // Chiều cao ảnh heal.png (điều chỉnh nếu cần)
constexpr float HEAL_ITEM_DROP_SPEED = 80.0f; // Tốc độ rơi của heal.png (pixel/giây)

// --- Gameplay & Waves ---
constexpr int INITIAL_MISSILE_COUNT = 1; // Số tên lửa ban đầu mỗi wave
constexpr int MAX_MISSILE_COUNT = 5; // Số tên lửa tối đa mỗi wave
constexpr int BASE_WAVES_UNTIL_INCREASE = 7; // Số wave cơ bản để tăng số lượng tên lửa
constexpr int RANDOM_WAVES_UNTIL_INCREASE = 6; // Số wave ngẫu nhiên cộng thêm (0-5)
constexpr Uint32 INITIAL_SPAWN_DELAY = 2000; // Độ trễ trước khi wave đầu tiên bắt đầu (ms)
constexpr Uint32 BASE_WAVE_DELAY = 3000; // Độ trễ cơ bản giữa các wave (ms)
constexpr Uint32 RANDOM_WAVE_DELAY = 2001; // Độ trễ ngẫu nhiên cộng thêm (0-2000 ms)
constexpr Uint32 MISSILE_SPAWN_INTERVAL = 300; // Khoảng cách thời gian tối thiểu giữa các tên lửa trong cùng wave (ms)
constexpr int SCORE_PER_MISSILE = 1; // Điểm cho mỗi tên lửa thường
constexpr int SCORE_PER_FAST_MISSILE = 2; // Điểm cho mỗi tên lửa nhanh
constexpr int SCORE_PER_SHARK = 5; // Điểm cho mỗi cá mập
constexpr int WAVE_START_FAST_MISSILE = 9; // Wave bắt đầu xuất hiện tên lửa nhanh
constexpr int WAVE_INTERVAL_FAST_MISSILE = 3; // Số wave cách nhau để xuất hiện tên lửa nhanh
constexpr int WAVE_START_SHARK = 15; // Wave bắt đầu xuất hiện cá mập
constexpr int WAVE_INTERVAL_SHARK = 15; // Số wave cách nhau để xuất hiện cá mập

// --- Giao diện người dùng (UI) ---
const SDL_Rect PAUSE_BUTTON_RECT = {SCREEN_WIDTH - 50, 10, 40, 40}; // Nút Pause góc trên phải
constexpr int BUTTON_WIDTH = 200; // Chiều rộng chung của các nút menu
constexpr int BUTTON_HEIGHT = 50; // Chiều cao chung của các nút menu
// Vị trí Y của các thành phần trong màn hình Game Over/Pause
constexpr int GAMEOVER_TITLE_Y = 100;
constexpr int PAUSED_TITLE_Y = 200;
constexpr int SCORE_LABEL_Y = 260;
constexpr int HIGHSCORE_LABEL_Y = 300;
const SDL_Rect RESTART_BUTTON_RECT = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 340, BUTTON_WIDTH, BUTTON_HEIGHT };
const SDL_Rect BACK_TO_MENU_BUTTON_RECT_GAMEOVER = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 400, BUTTON_WIDTH, BUTTON_HEIGHT };
const SDL_Rect GIVE_UP_BUTTON_RECT = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 460, BUTTON_WIDTH, BUTTON_HEIGHT }; // Nút Give Up trong Pause
constexpr int VOLUME_LABEL_Y = 390; // Nhãn Volume trong Pause
const SDL_Rect VOLUME_SLIDER_RECT = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 420, BUTTON_WIDTH, 10 }; // Thanh trượt Volume trong Pause
const SDL_Rect VOLUME_KNOB_RECT = { VOLUME_SLIDER_RECT.x + (DEFAULT_VOLUME * VOLUME_SLIDER_RECT.w / 100) - 5, 415, 10, 20 }; // Núm Volume trong Pause
// Vị trí các nút trong Main Menu
const SDL_Rect PLAY_BUTTON_RECT = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 250, BUTTON_WIDTH, BUTTON_HEIGHT };
const SDL_Rect HIGHSCORE_BUTTON_RECT = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 320, BUTTON_WIDTH, BUTTON_HEIGHT };
const SDL_Rect SETTINGS_BUTTON_RECT = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 390, BUTTON_WIDTH, BUTTON_HEIGHT };
const SDL_Rect EXIT_BUTTON_RECT = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 460, BUTTON_WIDTH, BUTTON_HEIGHT };
const SDL_Rect BACK_BUTTON_RECT = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 500, BUTTON_WIDTH, BUTTON_HEIGHT }; // Nút Back chung
// Vị trí các thành phần trong màn hình Highscore/Settings
constexpr int HIGHSCORE_TITLE_Y_MENU = 100;
constexpr int HIGHSCORE_LIST_Y = 200;
constexpr int MAX_HIGHSCORES_DISPLAY = 5; // Số điểm cao tối đa hiển thị
constexpr int SETTINGS_TITLE_Y = 100;
const SDL_Rect VOLUME_SLIDER_RECT_SETTINGS = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 280, BUTTON_WIDTH, 10 }; // Thanh trượt Volume trong Settings
const SDL_Rect VOLUME_KNOB_RECT_SETTINGS = { VOLUME_SLIDER_RECT_SETTINGS.x + (DEFAULT_VOLUME * VOLUME_SLIDER_RECT_SETTINGS.w / 100) - 5, 275, 10, 20 }; // Núm Volume trong Settings
const int VOLUME_LABEL_Y_SETTINGS = VOLUME_SLIDER_RECT_SETTINGS.y - 40; // Nhãn Volume trong Settings
const SDL_Rect SENSITIVITY_SLIDER_RECT_SETTINGS = { (SCREEN_WIDTH - BUTTON_WIDTH) / 2, 380, BUTTON_WIDTH, 10 }; // Thanh trượt Sensitivity trong Settings
const SDL_Rect SENSITIVITY_KNOB_RECT_SETTINGS = { SENSITIVITY_SLIDER_RECT_SETTINGS.x + (int)(DEFAULT_SENSITIVITY * SENSITIVITY_SLIDER_RECT_SETTINGS.w / 100.0f) - 5, 375, 10, 20 }; // Núm Sensitivity trong Settings
const int SENSITIVITY_LABEL_Y_SETTINGS = SENSITIVITY_SLIDER_RECT_SETTINGS.y - 40; // Nhãn Sensitivity trong Settings

// --- THÊM HẰNG SỐ VỊ TRÍ ĐIỂM KHI CHƠI ---
constexpr int INGAME_SCORE_TEXT_PADDING_X = 15;   // Khoảng cách từ cạnh phải màn hình
constexpr int INGAME_SCORE_TEXT_Y = 40;           // Vị trí Y của dòng Score (tăng lên để thấp xuống)
constexpr int INGAME_HIGHSCORE_TEXT_Y_OFFSET = 3; // Khoảng cách dọc giữa Score và Highscore

// --- Khác ---
constexpr int CIRCLE_SEGMENTS = 36; // Số đoạn vẽ hình tròn
constexpr int ARC_SEGMENTS = 24; // Số đoạn vẽ cung tròn (khiên)

#endif // CONFIG_H