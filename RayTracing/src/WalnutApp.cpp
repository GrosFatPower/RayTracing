#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Timer.h"

#include "Renderer.h"

using namespace Walnut;

class ExampleLayer : public Walnut::Layer
{
public:
  virtual void OnUIRender() override
  {
    ImGui::Begin("Settings");

    ImGui::Text("Last render: %.3fms", m_LastRenderTime);
    if ( m_LastRenderTime > 0. )
      ImGui::Text("FPS: %.2ffps", 1./(m_LastRenderTime*0.001));
    else
      ImGui::Text("FPS: INF");

    if ( m_IsPaused )
    {
      if (ImGui::Button("Render"))
        m_IsPaused = 0;
    }
    else if (ImGui::Button("Pause"))
      m_IsPaused = 1;

    int numThreads = m_NumThreads;
    if ( ImGui::SliderInt("Nb Threads", &numThreads, 1, 24) && ( numThreads > 0 ) )
      m_NumThreads = numThreads;

    ImGui::End();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport");

    m_ViewportWidth = static_cast<uint32_t>(ImGui::GetContentRegionAvail().x);
    m_ViewportHeight = static_cast<uint32_t>(ImGui::GetContentRegionAvail().y);

    m_Renderer.SetNumThreads(m_NumThreads);
    auto image = m_Renderer.GetFinalImage();
    if (image)
      ImGui::Image(image->GetDescriptorSet(), { (float)image->GetWidth(), (float)image->GetHeight() },
        ImVec2(0, 1), ImVec2(1, 0));

    ImGui::End();
    ImGui::PopStyleVar();

    if ( !m_IsPaused )
      Render();
  }

  void Render()
  {
    Timer timer;

    m_Renderer.OnResize(m_ViewportWidth, m_ViewportHeight);
    m_Renderer.Render();

    m_LastRenderTime = timer.ElapsedMillis();
  }
private:
  Renderer m_Renderer;
  uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

  float m_LastRenderTime = 0.0f;
  int m_NumThreads = 1;
  bool m_IsPaused = 0;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
  Walnut::ApplicationSpecification spec;
  spec.Name = "Ray Tracing";

  Walnut::Application* app = new Walnut::Application(spec);
  app->PushLayer<ExampleLayer>();
  app->SetMenubarCallback([app]()
  {
    if (ImGui::BeginMenu("File"))
    {
      if (ImGui::MenuItem("Exit"))
      {
        app->Close();
      }
      ImGui::EndMenu();
    }
  });
  return app;
}
