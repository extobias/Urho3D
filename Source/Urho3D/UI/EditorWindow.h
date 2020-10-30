#pragma once

#include "../UI/ImGuiElement.h"

namespace Urho3D
{

class Model;
class ParticleEmitter;
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

    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    void HandleSceneLoaded(StringHash eventType, VariantMap& eventData);

    void CreateGuizmo();

    void SetVisible(bool visible);

    void SetPlotVar(int index, float value);

    void SetScene(Scene* scene);

    void SetOrthographic(bool orthographics);

    void SetCameraPosition(const Vector3& position);

    void SetCameraRotation(const Quaternion& rotation);

//    String debugText_;

    float plotVars_[4][100];

    int plotVarsOffset_[4];

private:

    void AttributeEdit(Serializable *c);

    bool VariantEdit(const StringHash& key, Variant& v);

    void AddComponentMenu(Node *node);

    void EditParticleEmitter(ParticleEmitter* emitter);

    void EditModelDebug(EditorModelDebug* model);

    void DebugModelSubPart();

    int FindModel(const String& name);

    int FindMaterial(const String& name);

    int FindSprite(const String& name);

    void DrawChild(Node* node, int &i);

    void DrawNodeTree();

    void DrawNodeSelected();

    void MoveCamera(float timeStep);

    void LoadResources();

    void ConfigResources();

    bool SaveScene();

    bool LoadScene();

    bool SavePrefab(Node* node);

    bool LoadPrefab(Node* node);

    ResourceContainer& FindContainer(const StringHash& type);

    void AttachCamera();

    SharedPtr<Node> cameraNode_;

    SharedPtr<EditorSelection> selection_;

    SharedPtr<EditorGuizmo> guizmo_;

    PODVector<int> currentMaterialList_;

    Vector3 hitPosition_{ Vector3::ZERO };

    EditorMode mode_ { SELECT_OBJECT };

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

    bool sceneLoading_;
};

}
