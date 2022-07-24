#include "Renderer.h"

#include "Walnut/Random.h"

#include "stb_image.h"

#include <stdlib.h>
#include <string>
#include <thread>

static Camera S_Cam;
static Scene  S_Scene;

#define EPSILON 0.001f

Renderer::Renderer()
: m_ImageData(nullptr)
, m_BackgroundData(nullptr)
, m_BackGroundWidth(0)
, m_BackGroundHeight(0)
, m_NumThreads(1)
{
  std::string filepath("../../../resources/img/nature.jpg");

  int width = 0, height = 0, channels = 0;
  unsigned char * imgData = stbi_load(filepath.c_str(), &m_BackGroundWidth, &m_BackGroundHeight, &channels, 3);

  if ( imgData && m_BackGroundWidth && m_BackGroundHeight )
  {
    m_BackgroundData = new uint32_t[m_BackGroundWidth * m_BackGroundHeight];

    unsigned bytePerPixel = channels;
    for (int y = 0; y < m_BackGroundHeight; y++)
    {
      for (int x = 0; x < m_BackGroundWidth; x++)
      {
        unsigned char* pixelOffset = imgData + (x + m_BackGroundWidth * y) * bytePerPixel;
        unsigned char r = ( pixelOffset[0] & 0xff );
        unsigned char g = ( pixelOffset[1] & 0xff );
        unsigned char b = ( pixelOffset[2] & 0xff );
        unsigned char a = ( channels >= 4 ) ? ( pixelOffset[3] & 0xff ) : 0xff;

        uint32_t color = ( a << 24 ) + ( b << 16 ) + ( g << 8 ) + ( r );
        m_BackgroundData[x + m_BackGroundWidth * y] = color;
      }
    }
  }

  stbi_image_free(imgData);
}

Renderer::~Renderer()
{
  delete[] m_ImageData;
  delete[] m_BackgroundData;

  m_ImageData = nullptr;
  m_BackgroundData = nullptr;
}

void Renderer::OnResize(uint32_t width, uint32_t height)
{
  if (m_FinalImage)
  {
    // No resize necessary
    if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
      return;

    m_FinalImage->Resize(width, height);
  }
  else
  {
    m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
  }

  delete[] m_ImageData;
  m_ImageData = new uint32_t[width * height];
}

void Renderer::RenderPixels( Renderer * myRenderer, uint32_t startx, uint32_t endx, uint32_t starty, uint32_t endy )
{
  float invWidth  = 1.f / (float) myRenderer -> m_FinalImage -> GetWidth();
  float invHeight = 1.f / (float) myRenderer -> m_FinalImage -> GetHeight();

  for (uint32_t y = starty; y < endy; y++)
  {
    for (uint32_t x = startx; x < endx; x++)
    {
      glm::vec2 coord = { x * invWidth, y * invHeight };
      coord = coord * 2.0f - 1.0f; // -1 -> 1
      myRenderer -> m_ImageData[x + y * myRenderer -> m_FinalImage -> GetWidth()] = myRenderer -> PerPixel(coord);
    }
  }
}

void Renderer::Render()
{
  S_Cam._Ratio      = m_FinalImage -> GetWidth() / (float) m_FinalImage -> GetHeight();
  S_Cam._DirV       = -S_Cam._Up;
  S_Cam._DirU       = glm::cross(S_Cam._Dir, S_Cam._DirV);
  S_Cam._FirstPoint = S_Cam._Orig + S_Cam._Dir * S_Cam._ViewPlane;

  std::vector<std::thread> threads;

  for ( int i = 0; i < m_NumThreads; ++i )
  {
    //int startx = ( m_FinalImage -> GetWidth() / m_NumThreads ) * i;
    //int endx = startx + ( m_FinalImage -> GetWidth() / m_NumThreads );
    //int starty = 0;
    //int endy = m_FinalImage -> GetHeight();
    int startx = 0;
    int endx = m_FinalImage -> GetWidth();
    int starty = ( m_FinalImage -> GetHeight() / m_NumThreads ) * i;
    int endy = starty + ( m_FinalImage -> GetHeight() / m_NumThreads );
    threads.emplace_back(std::thread(Renderer::RenderPixels, this, startx, endx, starty,endy));
  }

  for ( auto & thread : threads )
  {
    thread.join();
  }

  m_FinalImage->SetData(m_ImageData);
}

uint32_t Renderer::PerPixel(const glm::vec2 & coord)
{
  uint32_t color = FetchBackgroundColor(coord);

  glm::vec3 pos = S_Cam._FirstPoint + coord.x * S_Cam._Width * .5f * S_Cam._DirU + coord.y * ( S_Cam._Width / S_Cam._Ratio ) * .5f * S_Cam._DirV;

  glm::vec3 rayDirection = glm::normalize(pos - S_Cam._Orig);

  // Using a for loop with iterator
  double minDist = -1.f;
  for( MyShape * curShape : S_Scene._Shapes )
  {
    MySphere * curSphere = dynamic_cast<MySphere *>(curShape);
    if ( !curSphere )
      continue;

    float dist = 0.f;
    bool inter = IsIntersecting(S_Cam._Orig, rayDirection, *curSphere, dist);
    if ( !inter )
      continue;

    if ( ( minDist > 0. ) && ( dist > minDist ) )
      continue;
    minDist = dist;
        
    glm::vec3 hitpoint = S_Cam._Orig + rayDirection * dist;
    glm::vec3 normal = glm::normalize(hitpoint - curSphere -> _Center);
    glm::vec3 dirToLight = glm::normalize(S_Scene._Light._Pos - hitpoint);
    
    float dotNorm = glm::dot(normal, dirToLight);
    
    if ( dotNorm > 0.f )
    {
      unsigned char r = (unsigned char)( ( ( 0x000000ff & curSphere -> _Color )       ) * dotNorm );
      unsigned char g = (unsigned char)( ( ( 0x0000ff00 & curSphere -> _Color ) >> 8  ) * dotNorm );
      unsigned char b = (unsigned char)( ( ( 0x00ff0000 & curSphere -> _Color ) >> 16 ) * dotNorm );
      unsigned char a = 0xff;
      color = ( a << 24 ) + ( b << 16 ) + ( g << 8 ) + ( r );
    }
    else
      color = 0xff000000;
  }

  return color;
}

uint32_t Renderer::FetchBackgroundColor(const glm::vec2 & coord)
{
  uint32_t color = 0xff000000;

  if ( m_BackgroundData )
  {
    glm::vec2 imgCoord(coord);

    imgCoord.x += 1.;
    imgCoord.y += 1.;
    imgCoord.x /= 2.;
    imgCoord.y /= 2.;

    uint32_t posx = (uint32_t) ( imgCoord.x * m_BackGroundWidth );
    uint32_t posy = ( m_BackGroundHeight - 1 ) - (uint32_t) ( imgCoord.y * m_BackGroundHeight );

    if ( posx < 0 )
      posx = 0;
    if ( posx >= (uint32_t)m_BackGroundWidth )
      posx = m_BackGroundWidth - 1;

    if ( posy < 0 )
      posy = 0;
    if ( posy >= (uint32_t)m_BackGroundHeight )
      posy = m_BackGroundHeight - 1;

    color = m_BackgroundData[ posx + posy * m_BackGroundWidth ];
  }

  return color;
}

bool Renderer::IsIntersecting(const glm::vec3 & orig, const glm::vec3 & dir, const MySphere & sphere, float & minDist)
{
  glm::vec3 translatedOrig = orig - sphere._Center;

  // (bx^2 + by^2)t^2 + (2(axbx + ayby))t + (ax^2 + ay^2 - r^2) = 0
  // where
  // a = ray origin
  // b = ray direction
  // r = radius
  // t = hit distance
  float a = 1;//glm::dot(rayDirection, rayDirection);
  float b = 2.0f * glm::dot(translatedOrig, dir);
  float c = glm::dot(translatedOrig, translatedOrig) - sphere._Radius * sphere._Radius;

  // Quadratic forumula discriminant:
  // b^2 - 4ac
  float delta = b * b - 4.0f * a * c;

  if ( delta > EPSILON )
  {
    minDist = ( -b - glm::sqrt(delta) ) / ( 2.f * a );
  }
  else if ( delta > 0.)
  {
    minDist = -b / ( 2.f * a );
  }
  else
    return false;

  return true;
}

Scene::Scene()
{
  MySphere * sphere = new MySphere;
  sphere -> _Center = glm::vec3(-.5f, -.5f, 2.f);
  sphere -> _Radius = 0.5f;
  sphere -> _Color  = 0xffff0000;
  _Shapes.push_back(sphere);

  sphere = new MySphere;
  sphere -> _Center = glm::vec3(1.5f, 1.f, 3.f);
  sphere -> _Radius = 0.2f;
  sphere -> _Color  = 0xff00ff00;
  _Shapes.push_back(sphere);

  sphere = new MySphere;
  sphere -> _Center = glm::vec3(-1.5f, 2.f, 5.f);
  sphere -> _Radius = 0.1f;
  sphere -> _Color  = 0xff0000ff;
  _Shapes.push_back(sphere);

  sphere = new MySphere;
  sphere -> _Center = glm::vec3(-.8f, -2.f, 6.f);
  sphere -> _Radius = 0.2f;
  sphere -> _Color  = 0xff00ffff;
  _Shapes.push_back(sphere);

  sphere = new MySphere;
  sphere -> _Center = glm::vec3(.2f, 2.f, 10.f);
  sphere -> _Radius = 0.8f;
  sphere -> _Color  = 0xffffff00;
  _Shapes.push_back(sphere);
}

Scene::~Scene()
{
  for( MyShape * curShape : _Shapes )
    delete curShape;
}
