#pragma once

#include "Walnut/Image.h"

#include <memory>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <glm/glm.hpp>

class Renderer;

struct Camera
{
  glm::vec3  _Orig;
  glm::vec3  _Dir;
  glm::vec3  _Up;
  glm::vec3  _FirstPoint;
  glm::vec3  _DirU;
  glm::vec3  _DirV;
  float      _IncrX;
  float      _IncrY;
  float      _ViewPlane;
  float      _Width;
  float      _Ratio;

  Camera() 
  : _Orig(0.f, 0.f, -2.f)
  , _Dir(0.f, 0.f, 1.f)
  , _Up(0.f, 1.f, 0.f)
  , _FirstPoint(0.f, 0.f, 0.f)
  , _DirU(0.f, 0.f, 0.f)
  , _DirV(0.f, 0.f, 0.f)
  , _IncrX(0.)
  , _IncrY(0.)
  , _ViewPlane(2.f)
  , _Width(2.f)
  , _Ratio(1.f)
  {}
};

class MyShape
{
public:
  MyShape() = default;
  virtual ~MyShape() {}
};

class MySphere : public MyShape
{
public:
  glm::vec3  _Center;
  float      _Radius;
  uint32_t   _Color;

  MySphere()
  : _Center(0.f, 0.f, 0.f)
  , _Radius(1.f)
  , _Color(0xffffffff)
  {
  }

  virtual ~MySphere() {}
};

struct PointLight
{
  glm::vec3  _Pos;

  PointLight()
  : _Pos(-3.f, -3.f, -3.f)
  {}
};

class Scene
{
public:
  Scene();
  ~Scene();

  PointLight              _Light;
  std::vector<MyShape * > _Shapes;
};

struct ThreadData
{
  Renderer * _Renderer;
  uint32_t   _Width;
  uint32_t   _Height;
  uint32_t   _StartX;
  uint32_t   _EndX;
  uint32_t   _StartY;
  uint32_t   _EndY;
};

class Renderer
{
public:
  Renderer();
  ~Renderer();

  void OnResize(uint32_t width, uint32_t height);
  void Render();

  std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }

  void SetNumThreads( int numThreads ) { if ( numThreads > 0 ) m_NumThreads = numThreads; }
  int GetNumThreads() const { return m_NumThreads; }

private:
  static void RenderPixels( ThreadData td );

  uint32_t PerPixel(const glm::vec2 & coord);

  uint32_t FetchBackgroundColor(const glm::vec2 & coord);

  bool IsIntersecting(const glm::vec3 & orig, const glm::vec3 & dir, const MySphere & sphere, float & minDist);

private:
  std::shared_ptr<Walnut::Image> m_FinalImage;
  uint32_t*                      m_ImageData = nullptr;
  uint32_t*                      m_BackgroundData = nullptr;
  int                            m_BackGroundWidth;
  int                            m_BackGroundHeight;
  int                            m_NumThreads;
};
