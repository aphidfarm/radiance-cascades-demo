#include "game.h"

void Game::setup() {
  boxPosition = { (float)SCREEN_WIDTH/2, (float)SCREEN_HEIGHT/2 };
  boxSize = 50;
  debug = false;
  tool = BRUSH;

  rainbowShader = LoadShader(0, TextFormat("res/shaders/rainbow.frag", GLSL_VERSION));
  maskShader = LoadShader(0, TextFormat("res/shaders/mask.frag", GLSL_VERSION));

  brush = LoadImage("res/brush_circle.png");
  brushTex = LoadTextureFromImage(brush);

  canvas = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BACKGROUND_COLOR);
  canvasTex = LoadTextureFromImage(canvas);
}

void Game::update() {
  time = GetTime();
  SetShaderValue(rainbowShader, GetShaderLocation(rainbowShader, "uTime"), &time, SHADER_UNIFORM_FLOAT);


  Vector2 resolution = { SCREEN_WIDTH, SCREEN_HEIGHT };
  SetShaderValue(maskShader, GetShaderLocation(maskShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);


  Vector2 mousePos   = { static_cast<float>(GetMouseX()),  static_cast<float>(GetMouseY()) };
  SetShaderValue(maskShader, GetShaderLocation(maskShader, "uMousePos"), &mousePos, SHADER_UNIFORM_VEC2);
}

void Game::render() {
  ClearBackground(PINK);

  BeginShaderMode(maskShader);
    // SetShaderValueTexture() has to be called while the shader is enabled
    SetShaderValueTexture(maskShader, GetShaderLocation(maskShader, "uOcclusionMask"), canvasTex);
    DrawTexture(canvasTex, 0, 0, WHITE);
  EndShaderMode();

  BeginShaderMode(rainbowShader);
    DrawRectanglePro((Rectangle){ boxPosition.x, boxPosition.y, boxSize, boxSize },
                     (Vector2){ boxSize/2, boxSize/2 },
                     0,
                     MAROON);
  EndShaderMode();

  for (Rectangle* r : walls) {
    DrawRectanglePro(*r, (Vector2){ 0, 0 }, 0, BLACK);
    // DrawLine(GetMouseX(), GetMouseY(), r->x, r->y, RED);
    // DrawLine(GetMouseX(), GetMouseY(), r->x+r->width, r->y, RED);
    // DrawLine(GetMouseX(), GetMouseY(), r->x, r->y+r->height, RED);
    // DrawLine(GetMouseX(), GetMouseY(), r->x+r->width, r->y+r->height, RED);
  }

  if (tool == BRUSH) {
    DrawTextureEx(brushTex,
                (Vector2){ (float)(GetMouseX() - brush.width/2*BRUSH_SCALE),
                           (float)(GetMouseY() - brush.height/2*BRUSH_SCALE) },
                0.0,
                BRUSH_SCALE,
                BLACK);
  } else {
    DrawRectanglePro(boxToolInfo.rect,
                     (Vector2){ 1, 1 },
                     0,
                     BLACK);
  }
}

void Game::renderUI() {
  std::string toolstr = "";
  if (tool == BRUSH) {
    toolstr = "Brush";
  } else if (tool == BOX) {
    toolstr = "Box";
  }
  DrawText(TextFormat("%s", toolstr.c_str()), 0, 0, 1, BLACK);

  if (debug) {
    DrawText(TextFormat("%i FPS",    GetFPS()), 0, 8, 1, BLACK);
  }
}

void Game::processKeyboardInput() {
  if (IsKeyPressed(KEY_F3)) debug = !debug;
  if (IsKeyPressed(KEY_C)) {
    // clear canvas
    std::cout << "Clearing canvas." << std::endl;
    canvas = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BACKGROUND_COLOR);
    UnloadTexture(canvasTex);
    canvasTex = LoadTextureFromImage(canvas);

    // clear walls
    std::cout << "Removing " << walls.size() << " walls." << std::endl;
    // for (int i = walls.size(); i > 0; i--) {
    for (int i = 0; i < walls.size(); i++) {
      delete walls[i];
      walls.erase(walls.begin() + i);
      i--;
    }
  }

  if (IsKeyPressed(KEY_ONE)) tool = BRUSH;
  if (IsKeyPressed(KEY_TWO)) tool = BOX;

  if (IsKeyDown(KEY_W)) boxPosition.y -= 80.0f * GetFrameTime();
  if (IsKeyDown(KEY_A)) boxPosition.x -= 80.0f * GetFrameTime();
  if (IsKeyDown(KEY_S)) boxPosition.y += 80.0f * GetFrameTime();
  if (IsKeyDown(KEY_D)) boxPosition.x += 80.0f * GetFrameTime();
}

void Game::processMouseInput() {
  // switch case doesnt work?
  if (tool == BRUSH) {
    if (IsMouseButtonDown(0)) {
      ImageDraw(&canvas,
                brush,
                (Rectangle){ 0, 0, (float)canvas.width, (float)canvas.height },
                (Rectangle){ static_cast<float>(GetMouseX() - brush.width/2*BRUSH_SCALE), static_cast<float>(GetMouseY() - brush.height/2*BRUSH_SCALE), static_cast<float>(brush.width*BRUSH_SCALE), static_cast<float>(brush.height*BRUSH_SCALE) },
                BLACK);
      UnloadTexture(canvasTex);
      canvasTex = LoadTextureFromImage(canvas);
    }
  } else if (tool == BOX) {
    if (IsMouseButtonPressed(0)) {
      boxToolInfo.rect.x = GetMouseX();
      boxToolInfo.rect.y = GetMouseY();
    }
    if (IsMouseButtonDown(0)) {
      boxToolInfo.rect.width  = (GetMouseX() - boxToolInfo.rect.x);
      boxToolInfo.rect.height = (GetMouseY() - boxToolInfo.rect.y);
    }
    if (IsMouseButtonReleased(0)) {
      Rectangle* r = new Rectangle;
      r->x      = boxToolInfo.rect.x-1; // there's a one-pixel discrepancy for some reason..?
      r->y      = boxToolInfo.rect.y-1; // there's a one-pixel discrepancy for some reason..?
      r->width  = boxToolInfo.rect.width;
      r->height = boxToolInfo.rect.height;
      walls.push_back(r);
      boxToolInfo.rect.width  = 0;
      boxToolInfo.rect.height = 0;
    }
  }
}
