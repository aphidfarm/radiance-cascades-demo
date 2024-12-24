#include "game.h"

#define MOUSE_VECTOR Vector2{ static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()) }
#define RELOAD_CANVAS() UnloadTexture(canvas.tex); \
                        canvas.tex = LoadTextureFromImage(canvas.img);

//                                           no. of lights, distance from centre
void placeLights(std::vector<Light>* lights, int N = 4, float d = 256.0) {
  for (float i = 0; i < N; i++) {
    float t = (i+1) * (PI*2/N) + 0.1;
    Light l;
    l.position = Vector2{ SCREEN_WIDTH/2 + std::sin(t) * d, SCREEN_HEIGHT/2 + std::cos(t) * d};
    l.color    = Vector3{ std::sin(t), std::cos(t), 1.0 };
    l.radius   = 300;
    lights->push_back(l);
  }
}

Game::Game() {
  debug         = false;
  mode          = DRAWING;

  currentToolIcon = LoadTextureFromImage(LoadImage("res/textures/icons/drawing.png"));

  lightingShader = LoadShader(0, "res/shaders/lighting.frag");
  if (!IsShaderValid(lightingShader)) {
    printf("lightingShader is broken!!\n");
    UnloadShader(lightingShader);
    lightingShader = LoadShader(0, "res/shaders/broken.frag");
  }

  cascadeAmount = 512;

  cursor.img = LoadImage("res/textures/cursor.png");
  cursor.tex = LoadTextureFromImage(cursor.img);

  brush.img = LoadImage("res/textures/brush.png");
  brush.tex = LoadTextureFromImage(brush.img);
  brush.scale = 0.25;

  canvas.img = LoadImage("res/textures/canvas.png");
  canvas.tex = LoadTextureFromImage(canvas.img);

  placeLights(&lights);

  HideCursor();
}

void Game::update() {
  time = GetTime();
  SetShaderValue(lightingShader,    GetShaderLocation(lightingShader, "uTime"), &time, SHADER_UNIFORM_FLOAT);

  Vector2 resolution = { SCREEN_WIDTH, SCREEN_HEIGHT };
  resolution *= GetWindowScaleDPI();
  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);

  Vector2 mouse = MOUSE_VECTOR * GetWindowScaleDPI();
  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uPlayerLocation"), &mouse, SHADER_UNIFORM_VEC2);

  int lightsAmount = lights.size();
  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uLightsAmount"), &lightsAmount, SHADER_UNIFORM_INT);
  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uCascadeAmount"), &cascadeAmount, SHADER_UNIFORM_INT);

  int viewing = 0;
  if (mode == VIEWING) viewing = 1;
  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uViewing"), &viewing, SHADER_UNIFORM_INT);

  for (int i = 0; i < lights.size(); i++) {
    Vector2 position = lights[i].position * GetWindowScaleDPI();
    float   radius    = lights[i].radius * GetWindowScaleDPI().x;
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, TextFormat("lights[%i].position", i)), &position, SHADER_UNIFORM_VEC2);
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, TextFormat("lights[%i].color",    i)), &lights[i].color,    SHADER_UNIFORM_VEC3);
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, TextFormat("lights[%i].radius",   i)), &radius,   SHADER_UNIFORM_FLOAT);
   }
}

void Game::render() {
  ClearBackground(PINK);

  BeginShaderMode(lightingShader);
    // <!> SetShaderValueTexture() has to be called while the shader is enabled
    SetShaderValueTexture(lightingShader, GetShaderLocation(lightingShader, "uOcclusionMask"), canvas.tex);
    DrawTexture(canvas.tex, 0, 0, WHITE);
  EndShaderMode();
}

void Game::renderUI() {
  float v = GetTime() - timeSinceModeSwitch;
  if (v > 1.0) v = 1.0;

  std::string str = "DRAWING";
  if (mode == LIGHTING)     str = "LIGHTING";
  else if (mode == VIEWING) str = "VIEWING";

  DrawTexture(currentToolIcon, 0, SCREEN_HEIGHT-8, WHITE);

  DrawText(str.c_str(), 12, SCREEN_HEIGHT - 8, 1, ColorFromNormalized(Vector4{1.0, 1.0, 1.0, static_cast<float>(1.0 - v)}));

  switch (mode) {
    case DRAWING:
      DrawTextureEx(brush.tex,
                    Vector2{ (float)(GetMouseX() - brush.tex.width  / 2 * brush.scale),
                             (float)(GetMouseY() - brush.tex.height / 2 * brush.scale) },
                    0.0,
                    brush.scale,
                    Color{ 0, 0, 0, 64} );
      break;
    case LIGHTING:
      for (int i = 0; i < lights.size(); i++) {
        DrawCircleLinesV(lights[i].position, lights[i].radius / 64 * 2, GREEN);
       }

      Color col    = ColorFromNormalized(Vector4{ std::sin(time), std::cos(time), 1.0, 0.5 });
      DrawCircleLines(GetMouseX(), GetMouseY(), brush.scale*600, col);
      break;
  }

  DrawTextureEx(cursor.tex,
                Vector2{ (float)(GetMouseX() - cursor.img.width / 2 * CURSOR_SIZE),
                         (float)(GetMouseY() - cursor.img.height/ 2 * CURSOR_SIZE) },
                0.0,
                CURSOR_SIZE,
                WHITE);

  if (!debug) return;

  DrawText(TextFormat("%i FPS",    GetFPS()),      0, 0,  1, GREEN);
  DrawText(TextFormat("%i lights", lights.size()), 0, 8,  1, GREEN);

  if      (mode == DRAWING)  DrawText(TextFormat("%f brush scale",           brush.scale),           0, 32, 1, GREEN);
  else if (mode == LIGHTING) DrawText(TextFormat("%f light size (diameter)", brush.scale * 600 * 2), 0, 32, 1, GREEN);

  DrawText(TextFormat("%i cascades", cascadeAmount), 0, 40, 1, GREEN);
}

void Game::processKeyboardInput() {
  auto changeMode = [this](Mode m) {
    mode = m;
    timeSinceModeSwitch = GetTime();
    if (m == DRAWING)
      currentToolIcon = LoadTextureFromImage(LoadImage("res/textures/icons/drawing.png"));
    else if (m == LIGHTING)
      currentToolIcon = LoadTextureFromImage(LoadImage("res/textures/icons/lighting.png"));
    else
      currentToolIcon = LoadTextureFromImage(LoadImage("res/textures/icons/viewing.png"));
  };

  if (IsKeyPressed(KEY_ONE))   changeMode(DRAWING);
  if (IsKeyPressed(KEY_TWO))   changeMode(LIGHTING);
  if (IsKeyPressed(KEY_THREE)) changeMode(VIEWING);

  if (IsKeyPressed(KEY_F3))  debug = !debug;
  if (IsKeyPressed(KEY_F12)) {
    printf("Taking screenshot.\n");
    if (!DirectoryExists("screenshots")) MakeDirectory("screenshots");
    TakeScreenshot("screenshots/screenshot.png");
  }

  // clearing
  if (IsKeyPressed(KEY_C)) {
    switch (mode) {
      case DRAWING:
        printf("Clearing canvas.\n");
        canvas.img = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
        RELOAD_CANVAS();
        break;
      case LIGHTING:
        printf("Clearing lights.\n");
        lights.clear();
        break;
    }
  }

  // replacing
  if (IsKeyPressed(KEY_R)) {
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
      // reloading
        printf("Reloading shaders.\n");
        UnloadShader(lightingShader);
        lightingShader = LoadShader(0, "res/shaders/lighting.frag");
        if (!IsShaderValid(lightingShader)) {
          UnloadShader(lightingShader);
          lightingShader = LoadShader(0, "res/shaders/broken.frag");
        }
    } else if (mode == DRAWING) {
      printf("Replacing canvas.\n");
      canvas.img = LoadImage("res/textures/canvas.png");
      RELOAD_CANVAS();
    } else if (mode == LIGHTING) {
      printf("Replacing lights.\n");
      lights.clear();
      placeLights(&lights);
    }
  }
}

void Game::processMouseInput() {
  if (IsKeyDown(KEY_LEFT_CONTROL) && debug) {
    int amp = 50;
    if (IsKeyDown(KEY_LEFT_SHIFT)) amp = 1;
    cascadeAmount += GetMouseWheelMove() * amp;
    if (cascadeAmount < 1) cascadeAmount = 1;
  } else {
    brush.scale += GetMouseWheelMove() / 100;
    if (brush.scale < 0.1) brush.scale = 0.1;
  }

  if (IsMouseButtonDown(2)) {
    for (int i = 0; i < lights.size(); i++) {
      if (Vector2Length(lights[i].position - MOUSE_VECTOR) < 16) {
        lights[i].position = Vector2{ static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()) };
      }
    }
  }

  switch (mode) {
    case DRAWING:
      if (IsMouseButtonDown(0)) {
        ImageDraw(&canvas.img,
                  brush.img,
                  Rectangle{ 0, 0, (float)canvas.img.width, (float)canvas.img.height },
                  Rectangle{ static_cast<float>(GetMouseX() - brush.img.width  / 2 * brush.scale),
                             static_cast<float>(GetMouseY() - brush.img.height / 2 * brush.scale),
                             static_cast<float>(brush.img.width  * brush.scale),
                             static_cast<float>(brush.img.height * brush.scale) },
                  BLACK);
        RELOAD_CANVAS();
      } else if (IsMouseButtonDown(1)) {
        ImageDraw(&canvas.img,
                  brush.img,
                  Rectangle{ 0, 0, (float)canvas.img.width, (float)canvas.img.height },
                  Rectangle{ static_cast<float>(GetMouseX() - brush.img.width  / 2 * brush.scale),
                             static_cast<float>(GetMouseY() - brush.img.height / 2 * brush.scale),
                             static_cast<float>(brush.img.width  * brush.scale),
                             static_cast<float>(brush.img.height * brush.scale) },
                  WHITE);
        RELOAD_CANVAS();
      }
      break;
    case LIGHTING:
      if (IsMouseButtonPressed(0)) {
        Light l;
        l.position = MOUSE_VECTOR;
        l.color    = Vector3{ std::sin(time), std::cos(time), 1.0 };
        l.radius   = brush.scale * 600;
        lights.push_back(l);
      } else if (IsMouseButtonDown(1)) {
        for (int i = 0; i < lights.size(); i++) {
          if (Vector2Length(lights[i].position - MOUSE_VECTOR) < 16) {
            lights.erase(lights.begin() + i);
          }
        }
      }
    break;
  }
}
