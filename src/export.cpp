#include "export.hpp"
#include <jansson.h>


Mesh::Mesh(const aiMesh* mesh, Model* model)
{
    Index = (unsigned)model->Meshes.size();
    vector<float> vertices(mesh->mNumVertices * model->VertexSize);
    for(int i = 0; i < mesh->mNumVertices; i++)
    {
        int ugly = 0;
        if(mesh->HasPositions())
        {
            copy_n(&mesh->mVertices[i].x, 3, vertices.data() + i * model->VertexSize + ugly);
            ugly += 3;
        }
        
        if(mesh->HasNormals())
        {
            copy_n(&mesh->mNormals[i].x, 3, vertices.data() + i * model->VertexSize + ugly);
            ugly += 3;
        }
        
        if(mesh->HasTangentsAndBitangents())
        {
            copy_n(&mesh->mTangents[i].x, 3, vertices.data() + i * model->VertexSize + ugly);
            copy_n(&mesh->mBitangents[i].x, 3, vertices.data() + i * model->VertexSize + ugly + 3);
            ugly += 6;
        }
        
        if(mesh->HasTextureCoords(0))
        {
            copy_n(&mesh->mTextureCoords[0][i].x, 3, vertices.data() + i * model->VertexSize + ugly);
            ugly += 2;
        }
    }
    
    vector<char> indices(mesh->mNumFaces * 3 * model->IndexSize);
    
    if(model->IndexSize == 1)
    {
        uint8_t* indice = (uint8_t*)indices.data();
        for(int i = 0; i < mesh->mNumFaces; i++)
        {
            copy_n(mesh->mFaces[i].mIndices, 3, indice + i * 3);
        }
    }
    else if(model->IndexSize == 2)
    {
        uint16_t* indice = (uint16_t*)indices.data();
        for(int i = 0; i < mesh->mNumFaces; i++)
        {
            copy_n(mesh->mFaces[i].mIndices, 3, indice + i * 3);
        }
    }
    
    model->Write(vertices);
    IndexStart = model->Write(indices);
    IndexCount = mesh->mNumFaces * 3;
    
    Material = model->Materials[mesh->mMaterialIndex].get();
}

Model::Model(const aiScene* scene)
: VertexCount(0), IndexCount(0), IndexSize(0), IndexOffset(0), VertexOffset(0), VertexSize(0)
{
    Name = scene->mRootNode->mName.C_Str();
    
    for(int i = 0; i < scene->mNumMeshes; i++)
    {
        auto mesh = scene->mMeshes[i];
        VertexCount += mesh->mNumVertices;
        IndexCount += mesh->mNumFaces * 3;
        IndexSize = max<unsigned>(IndexSize, IndexCount > numeric_limits<uint8_t>::max() ? 2 : 1);
        unsigned vertSize = 0;
        vector<Attribute> attributes;
        
        if(mesh->HasPositions())
        {
            attributes.emplace_back("Position", 3, vertSize * sizeof(float));
            vertSize += 3;
        }
        
        if(mesh->HasNormals())
        {
            attributes.emplace_back("Normal", 3, vertSize * sizeof(float));
            vertSize += 3;
        }
        
        if(mesh->HasTangentsAndBitangents())
        {
            attributes.emplace_back("Tangent", 3, vertSize * sizeof(float));
            attributes.emplace_back("Bitangent", 3, (vertSize + 3) * sizeof(float));
            vertSize += 6;
        }
        
        if(mesh->HasTextureCoords(0))
        {
            attributes.emplace_back("UV0", 2, vertSize * sizeof(float));
            vertSize += 2;
        }
        
        if(i == 0)
        {
            for(auto&& attr : attributes)
            {
                Add(unique_ptr<Attribute>(new Attribute(attr)));
            }
            VertexSize = vertSize;
        }
        else
        {
            assert(VertexSize == vertSize);
            assert(equal(attributes.begin(), attributes.end(), Attributes.begin(), [](const Attribute& lhs, const unique_ptr<Attribute>& rhs){
                return lhs == *(rhs.get());
            }));
        }
    }
    
    _Indices.reserve(IndexCount);
    _Vertices.reserve(VertexCount);
    
    for(int i = 0; i < scene->mNumMaterials; i++)
    {
        Add(unique_ptr<Material>(new Material(scene->mMaterials[i], this)));
    }
    
    for(int i = 0; i < scene->mNumMeshes; i++)
    {
        Add(unique_ptr<Mesh>(new Mesh(scene->mMeshes[i], this)));
    }
    
    Add(unique_ptr<Object>(new Object(scene->mRootNode, this)));
}



void Model::Add(unique_ptr<Object>&& obj)
{
    ObjectsMap.emplace(obj->Name, obj.get());
    Objects.emplace_back(move(obj));
}

void Model::Add(unique_ptr<Material>&& mat)
{
    MaterialsMap.emplace(mat->Name, mat.get());
    Materials.emplace_back(move(mat));
}

void Model::Add(unique_ptr<Mesh>&& mesh)
{
    Meshes.emplace_back(move(mesh));
}

void Model::Add(unique_ptr<Attribute>&& attr)
{
    AttributesMap.emplace(attr->Name, attr.get());
    Attributes.emplace_back(move(attr));
}

long long Model::Write(const vector<float>& vertices)
{
    auto offset = _Vertices.size();
    _Vertices.insert(_Vertices.end(), vertices.begin(), vertices.end());
    return offset;
}

long long Model::Write(const vector<char>& indices)
{
    auto offset = _Indices.size() / IndexSize;
    _Indices.insert(_Indices.end(), indices.begin(), indices.end());
    return offset;
}



Material::Material(const aiMaterial* material, Model* model)
: Roughness(0), SpecularPower(0), DiffuseCoeff(0), AmbientCoeff(0), FresnelPower(0)
{
    Index = (unsigned)model->Materials.size();
    aiString name;
    
    material->Get(AI_MATKEY_NAME, name);
    Name = string(name.C_Str());
    
    if(material->GetTexture(aiTextureType_DIFFUSE, 0, &name) == aiReturn_SUCCESS)
    {
        DiffuseTextureName = name.C_Str();
    }
    
    if(material->GetTexture(aiTextureType_NORMALS, 0, &name) == aiReturn_SUCCESS)
    {
        NormalTextureName = name.C_Str();
    }
    
    if(material->GetTexture(aiTextureType_SPECULAR, 0, &name) == aiReturn_SUCCESS)
    {
        SpecularTextureName = name.C_Str();
    }
    
    material->Get(AI_MATKEY_SHININESS, Roughness);
    material->Get(AI_MATKEY_SHININESS_STRENGTH, SpecularPower);
    aiShadingMode shadingModel;
    material->Get(AI_MATKEY_SHADING_MODEL, shadingModel);
    unordered_map<int, string> modeLookup = {
        {aiShadingMode_Blinn, "Blinn"},
        {aiShadingMode_CookTorrance, "CookTorrance"},
        {aiShadingMode_Flat, "Flat"},
        {aiShadingMode_Fresnel, "Fresnel"},
        {aiShadingMode_Gouraud, "Gouraund"},
        {aiShadingMode_Minnaert, "Minnaert"},
        {aiShadingMode_NoShading, "None"},
        {aiShadingMode_OrenNayar, "OrenNayar"},
        {aiShadingMode_Phong, "Phong"},
        {aiShadingMode_Toon, "Toon"},
    };
    
    ShadingModel = modeLookup[(int)shadingModel];
}

Object::Object(const aiNode* node, Model* model)
{
    Name = node->mName.C_Str();
    for(int i = 0; i < node->mNumMeshes; i++)
    {
        Meshes.emplace_back(model->Meshes[node->mMeshes[i]].get());
    }
    
    copy_n(&node->mTransformation.a1, 16, glm::value_ptr(Transform));
}


json_t* Object::CreateJSON()
{
    auto obj = json_object();
    auto transformArray = json_array();
    for(int i = 0; i < 16; i++)
    {
        json_array_append_new(transformArray, json_real(glm::value_ptr(Transform)[i]));
    }
    json_object_set_new(obj, "Transform", transformArray);
    auto meshes = json_array();
    for(auto&& mesh : Meshes)
    {
        json_array_append_new(meshes, json_integer(mesh->Index));
    }
    json_object_set_new(obj, "Meshes", meshes);
    return obj;
}

json_t* Mesh::CreateJSON()
{
    auto obj = json_object();
    json_object_set_new(obj, "IndexStart", json_integer(IndexStart));
    json_object_set_new(obj, "IndexCount", json_integer(IndexCount));
    json_object_set_new(obj, "Material", json_string(Material->Name.c_str()));
    return obj;
}

json_t* Model::CreateJSON()
{
    string filename = Name + ".modeldata";
    fstream file(filename, fstream::out | fstream::binary);
    
    VertexOffset = 0;
    file.write((char*)(_Vertices.data()), sizeof(float) * _Vertices.size());
    IndexOffset = (unsigned)_Vertices.size() * sizeof(float);
    file.write((char*)(_Indices.data()), VertexSize * _Indices.size());
    
    auto obj = json_object();
    
    auto item  = json_string(Name.c_str());
    json_object_set_new(obj, "Name", item);
    
    item = json_string((Name + ".modeldata").c_str());
    json_object_set_new(obj, "Data", item);
    
    item = json_integer(VertexCount);
    json_object_set_new(obj, "VertexCount", item);
    
    item = json_integer(IndexCount);
    json_object_set_new(obj, "IndexCount", item);
    
    item = json_integer(IndexSize);
    json_object_set_new(obj, "IndexSize", item);
    
    item = json_integer(VertexOffset);
    json_object_set_new(obj, "VertexOffset", item);
    
    item = json_integer(IndexOffset);
    json_object_set_new(obj, "IndexOffset", item);
    
    item = json_integer(VertexSize);
    json_object_set_new(obj, "VertexSize", item);
    
    item = json_array();
    for(auto&& mesh : Meshes)
    {
        json_array_append_new(item, mesh->CreateJSON());
    }
    json_object_set_new(obj, "Meshes", item);
    
    item = json_object();
    for(auto&& mat : Materials)
    {
        json_object_set_new(item, mat->Name.c_str(), mat->CreateJSON());
    }
    json_object_set_new(obj, "Materials", item);
    
    item = json_object();
    for(auto&& attr : Attributes)
    {
        json_object_set_new(item, attr->Name.c_str(), attr->CreateJSON());
    }
    json_object_set_new(obj, "Attributes", item);
    
    item = json_object();
    for(auto&& obj : Objects)
    {
        json_object_set_new(item, obj->Name.c_str(), obj->CreateJSON());
    }
    json_object_set_new(obj, "Objects", item);
    
    return obj;
}

json_t* Material::CreateJSON()
{
    auto obj = json_object();
    
    auto item = json_string(Name.c_str());
    json_object_set_new(obj, "Name", item);
    
    item = json_string(ShadingModel.c_str());
    json_object_set_new(obj, "ShadingModel", item);
    
    item = json_string(DiffuseTextureName.c_str());
    json_object_set_new(obj, "DiffuseTexture", item);
    
    item = json_string(NormalTextureName.c_str());
    json_object_set_new(obj, "NormalTexture", item);
    
    item = json_string(SpecularTextureName.c_str());
    json_object_set_new(obj, "SpecularTexture", item);
    
    item = json_real(AmbientCoeff);
    json_object_set_new(obj, "AmbientCoeff", item);
    
    item = json_real(DiffuseCoeff);
    json_object_set_new(obj, "DiffuseCoeff", item);
    
    item = json_real(SpecularPower);
    json_object_set_new(obj, "SpecularPower", item);
    
    item = json_real(FresnelPower);
    json_object_set_new(obj, "FresnelPower", item);
    
    item = json_real(Roughness);
    json_object_set_new(obj, "Roughness", item);

    return obj;
}

json_t* Attribute::CreateJSON()
{
    auto obj = json_object();
    auto sizeObj = json_integer(Size);
    auto offsetObj = json_integer(Offset);
    
    json_object_set_new(obj, "Size", sizeObj);
    json_object_set_new(obj, "Offset", offsetObj);
    
    return obj;
}





int main(int argc, const char* args[])
{
    
    Assimp::Importer importer;
    unsigned flags = 0;
    flags |= aiProcess_Triangulate;
    flags |= aiProcess_RemoveRedundantMaterials;
    flags |= aiProcess_OptimizeGraph;
    flags |= aiProcess_OptimizeMeshes;
    flags |= aiProcess_JoinIdenticalVertices;
    flags |= aiProcess_ImproveCacheLocality;
    flags |= aiProcess_CalcTangentSpace;

    
    const auto scene = importer.ReadFile("Scene.obj", flags);
    
    Model model(scene);

    json_dump_file(model.CreateJSON(), (model.Name + ".model").c_str(), JSON_INDENT(4) | JSON_SORT_KEYS);
    
    
    cout << model.Name << endl;
	return 0;
}