#pragma once

#include "../UI/ImGuiElement.h"

namespace Urho3D
{

class Model;
class ParticleEmitter;
class EditorGuizmo;
class EditorModelDebug;

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

class URHO3D_API EditorSelection : public Object
{
    URHO3D_OBJECT(EditorSelection, Object);

public:

    explicit EditorSelection(Context* context);

    ~EditorSelection() override;

    void Add(unsigned id);

    void Clear();

    String ToString();

    void SetScene(Scene* scene) { scene_ = scene; }

    void SetTransform(const Matrix3x4& matrix);

    void SetDelta(const Matrix3x4& matrix);

    void Render();

    const PODVector<unsigned>& GetSelectedNodes() const { return selectedNodes_; }

    const Matrix3x4& GetTransform() const { return transform_; }

private:

    void UpdateTransform();

    bool PointAboveLine(Vector3 point, Vector3 p1, Vector3 p2);

    void PoligonPoints(Vector<Vector3>& points);

    Vector3 CalculateCentroid(const Vector<Vector3>& points);

    PODVector<unsigned> selectedNodes_;

    Matrix3x4 transform_;

    Scene* scene_;

//        unsigned selectedSubElementIndex_{ M_MAX_UNSIGNED };
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

    void SetCameraNode(Node* node);

    void HandleNodeSelected(StringHash eventType, VariantMap& eventData);

    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    void CreateGuizmo();

    void SetVisible(bool visible);

    void SetPlotVar(int index, float value);

    void SetScene(Scene* scene);

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

    SharedPtr<EditorSelection> selection_;

    SharedPtr<EditorGuizmo> guizmo_;

    PODVector<int> currentMaterialList_;

    Vector3 hitPosition_{ Vector3::ZERO };

    EditorMode mode_ { SELECT_OBJECT };

    Node* cameraNode_;

    ResourceDir resources_;

    ResourceMap modelResources_;

    String modelResourcesString_;

    ResourceMap materialResources_;

    String materialResourcesString_;

    ResourceMap spriteResources_;

    String spriteResourcesString_;

    int currentModel_;

    int currentSprite_;

    float yaw_;

    float pitch_;
};

}
