#pragma once

#include "../UI/ImGuiElement.h"

namespace Urho3D
{
    class Model;
    class ParticleEmitter;
    class ParticleEmitter2D;
    class EditorGuizmo;
    class EditorModelDebug;
    class EditorSelection;

struct ResourceFile
{
    String prefix;
    String path;
    String name;
    String ext;
};

using ResourceDir = HashMap<String, Vector<ResourceFile>>;

using ResourceMap = HashMap<String, ResourceFile>;

struct ResourceContainer
{
    ResourceContainer() {}

    ResourceContainer(const String& name, const String& dirfilter)
        : name_(name),
          dirFilter_(dirfilter)
    {
        type_ = StringHash(name);
    }

    ResourceContainer(const String& name, const String& dirfilter, const String& filter)
        : name_(name),
          dirFilter_(dirfilter)
    {
        type_ = StringHash(name);
        extFilter_.Push(filter);
    }

    ResourceContainer(const String& name, const String& dirfilter, const Vector<String>& filter)
        : name_(name),
          dirFilter_(dirfilter)
    {
        type_ = StringHash(name);
        extFilter_ = filter;
    }

    ResourceContainer(const String& name, const Vector<String>& filter)
        : name_(name)
    {
        type_ = StringHash(name);
        extFilter_ = filter;
    }

    int FindResourceIndex(const String& name) const
    {
        Vector<ResourceFile> values = resources_.Values();
        int index = 0;
        for (ResourceFile& resource: values)
        {
            if (resource.name == name)
            {
                return index;
            }
            index++;
        }
        return -1;
    }

    ResourceMap resources_;

    String resourcesString_;

    String name_;

    StringHash type_;

    String dirFilter_;

    Vector<String> extFilter_;

    int currentIndex_;
};

enum EditorMode
{
    SELECT_OBJECT,
    SELECT_MESH_VERTEX,
    SELECT_POLYGON_VERTEX
};

enum EditorNavMode
{
    NAV_FPS,
    NAV_LOOK_AT,
};

class URHO3D_API EditorWindow : public ImGuiElement
{
    URHO3D_OBJECT(EditorWindow, ImGuiElement);

public:
    /// Construct.
    explicit EditorWindow(Context* context);
    /// Destruct.
    ~EditorWindow() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    virtual void Render(float timeStep) override;

    bool IsWheelHandler() const override { return true; }

    void SetGuizmo(EditorGuizmo* guizmo) { guizmo_ = guizmo; }

    void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);
    
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    void HandleSceneLoaded(StringHash eventType, VariantMap& eventData);

    void CreateGuizmo();

    void SetVisible(bool visible);

    // void SetPlotVar(int index, float value);

    void SetPlotVar(int index, float value, float min = -100.0f, float max = 100.0f);

    void SetScene(Scene* scene);

    void SetOrthographic(bool orthographics);

    void SetCameraPosition(const Vector3& position);

    void SetCameraRotation(const Quaternion& rotation);

    void SetCameraEnabled(bool enable);

    EditorSelection* GetSelection() const { return selection_; }
//    String debugText_;

    float plotVars_[4][100];

    int plotVarsOffset_[4];

    int plotVarsRange_[4][2];

private:

    void AttributeEdit(Serializable *c);

    bool VariantEdit(const String& name, Variant& v, unsigned index = 0);

    void AddComponentMenu(Node *node);

    void EditParticleEmitter(ParticleEmitter* emitter);

    void EditParticleEmitter2D(ParticleEmitter2D* emitter);

    bool EditAttribute(const String& name, int& value, int totalWidth);

    bool EditAttribute(const String& name, float& value, int totalWidth);

    bool EditAttribute(const String& name, Vector2& value, int totalWidth);

    bool EditAttribute(const String& name, Vector3& value, int totalWidth);

    bool EditAttribute(const String& name, Color& value, int totalWidth);

    void EditModelDebug(EditorModelDebug* model);

    bool ImportFBX(const String& name, const String& path);

    void DebugModelSubPart();

    int FindModel(const String& name);

    int FindMaterial(const String& name);

    int FindSprite(const String& name);

    void DrawChild(Node* node, int &i);

    void DrawNodeTree();

    void DrawNodeSelected();

    void MoveCamera(float timeStep);

    void UpdateAnimations(float timeStep);

    void LoadResources();

    void ConfigResources();

    void ClearResources();

    bool SaveScene();

    bool LoadScene();

    bool SavePrefab(Node* node);

    bool LoadPrefab(Node* node);

    bool FindContainer(const StringHash& type, ResourceContainer& container);

    void AttachCamera();

    SharedPtr<Node> cameraNode_;

    SharedPtr<EditorSelection> selection_;

    SharedPtr<EditorGuizmo> guizmo_;

    PODVector<int> currentMaterialList_;

    Vector3 hitPosition_{ Vector3::ZERO };

    EditorMode mode_ { SELECT_OBJECT };

    EditorNavMode navMode_ { NAV_FPS };

    ResourceDir resources_;

    ResourceMap modelResources_;

    String modelResourcesString_;

    ResourceMap materialResources_;

    String materialResourcesString_;

    ResourceMap spriteResources_;

    String spriteResourcesString_;

    Vector<ResourceContainer> resourcesContainer_;

    int currentModel_;

    int currentSprite_;

    float yaw_;

    float pitch_;

    float theta_;

    float phi_;

    float camDistance_;

    bool sceneLoading_;

    bool fullscreen_;

    const char* hideLabel_ {"##hidelabel"};

    float timeStep_;
};

}
