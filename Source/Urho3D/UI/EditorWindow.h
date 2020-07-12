#pragma once

#include "../UI/ImGuiElement.h"

namespace Urho3D
{

class Model;
class ParticleEmitter;
class EditorGuizmo;
class EditorModelDebug;

static const StringHash E_EDITOR_NODE_SELECTED("EDITOR_NODE_SELECTED");
static const StringHash P_EDITOR_NODE_SELECTED("EDITOR_NODE_SELECTED_ID");

struct ResourceFile
{
        String prefix;
        String path;
        String name;
        String ext;
};

using ResourceDir = HashMap<String, Vector<ResourceFile>>;

using ResourceMap = HashMap<String, ResourceFile>;

enum EditorMode
{
    SELECT_OBJECT,
    SELECT_VERTEX
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

    virtual void Render(float timeStep);

    void SetGuizmo(EditorGuizmo* guizmo) { guizmo_ = guizmo; }

    void SetCameraNode(Node* node);

    void HandleNodeSelected(StringHash eventType, VariantMap& eventData);

    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    void CreateGuizmo();

    void SetVisible(bool visible);

    void SetPlotVar(int index, float value);

    void SetScene(Scene* scene);

    void AddSelectedNode(unsigned id);

//    String debugText_;

//    float plotVars_[4][100];

//    int plotVarsOffset_[4];

private:

    void AttributeEdit(Serializable *c);

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

    PODVector<int> currentMaterialList_;

    PODVector<unsigned> selectedNodes_;

    Vector3 hitPosition_{ Vector3::ZERO };

    EditorMode mode_ { SELECT_OBJECT };

    Node* cameraNode_;

    EditorGuizmo* guizmo_;

    ResourceDir resources_;

    ResourceMap modelResources_;

    String modelResourcesString_;

    ResourceMap materialResources_;

    String materialResourcesString_;

    ResourceMap spriteResources_;

    String spriteResourcesString_;

    int currentModel_;

    int currentSprite_;

    unsigned selectedNode_;

    unsigned selectedSubElementIndex_{ M_MAX_UNSIGNED };

    float yaw_;

    float pitch_;
};

}
