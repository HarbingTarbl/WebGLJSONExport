#include "export.hpp"
#include <jansson.h>


Mesh::Mesh(const aiMesh* mesh, Model* model)
{
    
    Name = mesh->mName.C_Str();
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
    else if(model->IndexSize == 4)
    {
        uint32_t* indice = (uint32_t*)indices.data();

        for(int i = 0; i < mesh->mNumFaces; i++)
        {
            copy_n(mesh->mFaces[i].mIndices, 3, indice + i * 3);
        }
    }
    
    model->Write(vertices);
    IndexOffset = model->Write(indices);
    IndexCount = mesh->mNumFaces * 3;
    VertexCount = mesh->mNumVertices;
    
    Material = model->Materials[mesh->mMaterialIndex].get();
}

Model::Model(const aiScene* scene)
: VertexCount(0), IndexCount(0),
IndexSize(0),
IndexOffset(0), VertexOffset(0), VertexSize(0)
{
    Name = scene->mRootNode->mName.C_Str();
    for(int i = 0; i < scene->mNumMeshes; i++)
    {
        auto mesh = scene->mMeshes[i];
        VertexCount += mesh->mNumVertices;
        IndexCount += mesh->mNumFaces * 3;
        if(IndexCount > numeric_limits<uint16_t>::max())
            IndexSize = 4 > IndexSize ? 4 : IndexSize;
        else if(IndexCount > numeric_limits<uint8_t>::max())
            IndexSize = 2 > IndexSize ? 2 : IndexSize;
        else
            IndexSize = 1 > IndexSize ? 1 : IndexSize;
        
        
        unsigned vertSize = 0;
        vector<Attribute> attributes;
        
        if(mesh->HasPositions())
        {
            attributes.emplace_back("position", attributes.size(), 3, vertSize * sizeof(float));
            vertSize += 3;
        }
        
        if(mesh->HasNormals())
        {
            attributes.emplace_back("normal", attributes.size(), 3, vertSize * sizeof(float));
            vertSize += 3;
        }
        
        if(mesh->HasTangentsAndBitangents())
        {
            attributes.emplace_back("tangent", attributes.size(), 3, vertSize * sizeof(float));
            attributes.emplace_back("bitangent", attributes.size(), 3, (vertSize + 3) * sizeof(float));
            vertSize += 6;
        }
        
        if(mesh->HasTextureCoords(0))
        {
            attributes.emplace_back("uv0", attributes.size(), 2, vertSize * sizeof(float));
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
    
    //_Indices.reserve(IndexCount);
    //_Vertices.reserve(VertexCount);
    
    for(int i = 0; i < scene->mNumMaterials; i++)
    {
        Add(unique_ptr<Material>(new Material(scene->mMaterials[i], this)));
    }
    
    for(int i = 0; i < scene->mNumMeshes; i++)
    {
        Add(unique_ptr<Mesh>(new Mesh(scene->mMeshes[i], this)));
    }
    
    Add(unique_ptr<Object>(new Object(scene->mRootNode, this)));
    for(int i = 0; i < scene->mRootNode->mNumChildren; i++)
    {
        Add(unique_ptr<Object>(new Object(scene->mRootNode->mChildren[i], this)));
    }
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
    auto offset = _Indices.size();
    _Indices.insert(_Indices.end(), indices.begin(), indices.end());
    return offset;
}



Material::Material(const aiMaterial* material, Model* model)
: Roughness(0), SpecularPower(0), DiffuseCoeff(0), AmbientCoeff(0), FresnelPower(0),
AmbientColor(0), DiffuseColor(0), SpecularColor(0)
{
    Index = (unsigned)model->Materials.size();
    aiString name;
    
    material->Get(AI_MATKEY_NAME, name);
    Name = string(name.C_Str());
    
    
    vector<string> mapTypes = {
        "none",
        "diffuse",
        "specular",
        "ambient",
        "emissive",
        "height",
        "normals",
        "shininess",
        "opacity",
        "displacement",
        "lightmap",
        "reflection",
        "unknown"
    };
    
    for (int i = 1; i < mapTypes.size(); i++) {
        auto k = material->GetTextureCount((aiTextureType)i);
        for(int j = 0; j < k; j++)
        {
            aiString path;
            material->GetTexture((aiTextureType)i, j, &path);
            auto f = Textures.find(mapTypes[i]);
            if(f == Textures.end())
            {
                f = Textures.emplace_hint(f, mapTypes[i] + to_string(j), vector<string>());
            }
            
            f->second.emplace_back(string(path.C_Str()));
        }
    }
    
    
    aiColor3D tempColor;
    
    material->Get(AI_MATKEY_SHININESS, Roughness);
    material->Get(AI_MATKEY_SHININESS_STRENGTH, SpecularPower);
    aiShadingMode shadingModel;
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
    
    material->Get(AI_MATKEY_COLOR_AMBIENT, tempColor);
    copy_n(&tempColor.r, 3, &AmbientColor.x);
    material->Get(AI_MATKEY_COLOR_DIFFUSE, tempColor);
    copy_n(&tempColor.r, 3, &DiffuseColor.x);
    material->Get(AI_MATKEY_COLOR_SPECULAR, tempColor);
    copy_n(&tempColor.r, 3, &SpecularColor.x);
    
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

void BoundingVolume::Union(const BoundingVolume &other)
{
    auto min = glm::min(Min(), other.Min());
    auto max = glm::max(Max(), other.Max());
    
    Center = (max + min) / 2.0f;
    Extents = max - Center;
}


json_t* BoundingVolume::CreateJSON()
{
    auto obj = json_object();
    auto item  = json_array();
    json_array_append_new(item, json_real(Center.x));
    json_array_append_new(item, json_real(Center.y));
    json_array_append_new(item, json_real(Center.z));
    json_object_set_new(obj, "center", item);
    
    item = json_array();
    json_array_append_new(item, json_real(Extents.x));
    json_array_append_new(item, json_real(Extents.y));
    json_array_append_new(item, json_real(Extents.z));
    
    json_object_set_new(obj, "extents", item);
    
    return obj;
}

json_t* Object::CreateJSON()
{
    auto obj = json_object();
    auto transformArray = json_array();
    for(int i = 0; i < 16; i++)
    {
        json_array_append_new(transformArray, json_real(glm::value_ptr(Transform)[i]));
    }
    json_object_set_new(obj, "transform", transformArray);
    auto meshes = json_array();
    for(auto&& mesh : Meshes)
    {
        json_array_append_new(meshes, json_integer(mesh->Index));
    }
    json_object_set_new(obj, "meshes", meshes);
    return obj;
}

json_t* Mesh::CreateJSON()
{
    auto obj = json_object();
    json_object_set_new(obj, "name", json_string(Name.c_str()));
    json_object_set_new(obj, "indexOffset", json_integer(IndexOffset));
    json_object_set_new(obj, "indexCount", json_integer(IndexCount));
    json_object_set_new(obj, "material", json_string(Material->Name.c_str()));
    return obj;
}

json_t* Model::CreateJSON()
{
    string filename = Name + ".modeldata";
    fstream file(filename, fstream::out | fstream::binary);
    
    VertexOffset = 0;
    file.write((char*)(_Vertices.data()), sizeof(float) * _Vertices.size());
    IndexOffset = (unsigned)_Vertices.size() * sizeof(float);
    
    unsigned long long iOffset = 0;
    if(IndexSize == 4)
    {
        for(int i = 0; i < Meshes.size(); i++)
        {
            auto elements = (uint32_t*)(_Indices.data() + Meshes[i]->IndexOffset);
            for(int k = 0; k < Meshes[i]->IndexCount; k++)
            {
                elements[k] += iOffset;
            }
            iOffset += Meshes[i]->VertexCount;
        }
        
    }
    else if(IndexSize == 2)
    {
        for(int i = 0; i < Meshes.size(); i++)
        {
            auto elements = (uint16_t*)(_Indices.data() + Meshes[i]->IndexOffset);
            for(int k = 0; k < Meshes[i]->IndexCount; k++)
            {
                elements[k] += iOffset;
            }
            iOffset += Meshes[i]->VertexCount;
        }
    }
    else
    {
        for(int i = 0; i < Meshes.size(); i++)
        {
            auto elements = (uint8_t*)(_Indices.data() + Meshes[i]->IndexOffset);
            for(int k = 0; k < Meshes[i]->IndexCount; k++)
            {
                elements[k] += iOffset;
            }
            iOffset += Meshes[i]->VertexCount;
        }
    }
    
    file.write((char*)(_Indices.data()), _Indices.size());
    
    auto obj = json_object();
    
    auto item  = json_string(Name.c_str());
    json_object_set_new(obj, "name", item);
    
    item = json_string((Name + ".modeldata").c_str());
    json_object_set_new(obj, "data", item);
    
    item = json_integer(VertexCount);
    json_object_set_new(obj, "vertexCount", item);
    
    item = json_integer(IndexCount);
    json_object_set_new(obj, "indexCount", item);
    
    item = json_integer(IndexSize);
    json_object_set_new(obj, "indexSize", item);
    
    item = json_integer(VertexOffset);
    json_object_set_new(obj, "vertexOffset", item);
    
    item = json_integer(IndexOffset);
    json_object_set_new(obj, "indexOffset", item);
    
    item = json_integer(VertexSize);
    json_object_set_new(obj, "vertexSize", item);
    
    item = json_array();
    for(auto&& mesh : Meshes)
    {
        json_array_append_new(item, mesh->CreateJSON());
    }
    json_object_set_new(obj, "meshes", item);
    
    item = json_object();
    for(auto&& mat : Materials)
    {
        json_object_set_new(item, mat->Name.c_str(), mat->CreateJSON());
    }
    json_object_set_new(obj, "materials", item);
    
    item = json_object();
    for(auto&& attr : Attributes)
    {
        json_object_set_new(item, attr->Name.c_str(), attr->CreateJSON());
    }
    json_object_set_new(obj, "attributes", item);
    
    item = json_object();
    for(auto&& obj : Objects)
    {
        json_object_set_new(item, obj->Name.c_str(), obj->CreateJSON());
    }
    json_object_set_new(obj, "objects", item);
    
    return obj;
}

static json_t* CreateJSON(const fvec3& vec)
{
    auto arr = json_array();
    json_array_append_new(arr, json_real(vec.x));
    json_array_append_new(arr, json_real(vec.y));
    json_array_append_new(arr, json_real(vec.z));
    return arr;
}


json_t* Material::CreateJSON()
{
    auto obj = json_object();
    
    auto item = json_string(Name.c_str());
    json_object_set_new(obj, "Name", item);
    json_object_set_new(obj, "ShadingModel", json_string(ShadingModel.c_str()));
    
    auto textureMap = json_object();
    for(auto&& kv : Textures)
    {
        for(int i = 0; i < kv.second.size(); i++)
        {
            json_object_set_new(textureMap, kv.first.c_str(), json_string(kv.second[i].c_str()));
        }
    }
    
    json_object_set_new(obj, "textures", textureMap);
    json_object_set_new(obj, "ambientCoeff", json_real(AmbientCoeff));
    json_object_set_new(obj, "diffuseCoeff", json_real(DiffuseCoeff));
    json_object_set_new(obj, "specularPower", json_real(SpecularPower));
    json_object_set_new(obj, "fresnelPower", json_real(FresnelPower));
    json_object_set_new(obj, "roughness", json_real(Roughness));
    json_object_set_new(obj, "diffuseColor", ::CreateJSON(DiffuseColor));
    json_object_set_new(obj, "ambientColor", ::CreateJSON(AmbientColor));
    json_object_set_new(obj, "specularColor", ::CreateJSON(SpecularColor));

    return obj;
}

json_t* Attribute::CreateJSON()
{
    auto obj = json_object();
    auto sizeObj = json_integer(Size);
    auto offsetObj = json_integer(Offset);
    
    json_object_set_new(obj, "Index", json_integer(Index));
    json_object_set_new(obj, "Size", sizeObj);
    json_object_set_new(obj, "Offset", offsetObj);
    
    return obj;
}





int main(int argc, const char* args[])
{
    if(argc < 2)
        return 1;
    
    Assimp::Importer importer;
    unsigned flags = 0;
    flags |= aiProcess_Triangulate;
    flags |= aiProcess_RemoveRedundantMaterials;
    flags |= aiProcess_OptimizeGraph;
    flags |= aiProcess_OptimizeMeshes;
    flags |= aiProcess_JoinIdenticalVertices;
    flags |= aiProcess_ImproveCacheLocality;
    flags |= aiProcess_CalcTangentSpace;
    flags |= aiProcess_FindInstances;
    flags |= aiProcess_FlipUVs;

    
    const auto scene = importer.ReadFile(args[1], flags);
    
    Model model(scene);

    json_dump_file(model.CreateJSON(), (model.Name + ".model").c_str(), JSON_INDENT(4) | JSON_SORT_KEYS);
    
    
    cout << model.Name << endl;
	return 0;
}