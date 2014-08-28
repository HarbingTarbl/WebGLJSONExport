//
//  export.hpp
//  WebGLJSONExport
//
//  Created by Michael Harris on 8/28/14.
//  Copyright (c) 2014 Michael. All rights reserved.
//

#ifndef WebGLJSONExport_export_hpp
#define WebGLJSONExport_export_hpp

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <exception>
#include <functional>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <numeric>

using namespace std;
using glm::fmat4;
using glm::fvec4;
using glm::fvec3;
using glm::fvec2;

class Material;
class Object;
class Attribute;
class Mesh;
struct json_t;

class Model
{
public:
    vector<unique_ptr<Mesh>> Meshes;
    vector<unique_ptr<Material>> Materials;
    vector<unique_ptr<Object>> Objects;
    vector<unique_ptr<Attribute>> Attributes;
    
    unordered_map<string, Material*> MaterialsMap;
    unordered_map<string, Object*> ObjectsMap;
    unordered_map<string, Attribute*> AttributesMap;
    
    string Name;
    string Data;
    unsigned VertexCount;
    unsigned IndexCount;
    unsigned IndexSize;
    unsigned VertexOffset;
    unsigned IndexOffset;
    unsigned VertexSize;
    
    
    Model(const aiScene* scene);
    
    void Add(unique_ptr<Object>&& obj);
    
    void Add(unique_ptr<Material>&& mat);
    
    void Add(unique_ptr<Mesh>&& mesh);
    
    void Add(unique_ptr<Attribute>&& attr);
    
    long long Write(const vector<float>& vertices);
    
    long long Write(const vector<char>& indices);
    
    json_t* CreateJSON();
    
private:
    vector<float> _Vertices;
    vector<char> _Indices;
};

class Mesh
{
public:
    unsigned long IndexStart;
    unsigned long IndexCount;
    unsigned Index;
    
    Material* Material;
    
    Mesh(const aiMesh* mesh, Model* model);
    
    json_t* CreateJSON();
};

class Attribute
{
public:
    string Name;
    unsigned Size;
    unsigned Offset;
    
    Attribute() = default;
    Attribute(const Attribute&) = default;
    Attribute(Attribute&&) = default;
    Attribute(const string& name, unsigned size, unsigned offset)
    : Name(name), Offset(offset), Size(size)
    {
        
    }
    
    json_t* CreateJSON();
    
    
    inline bool operator==(const Attribute& o) const
    {
        return Size == o.Size && Offset == o.Offset && Name == o.Name;
    }
};





class Object
{
public:
    string Name;
    vector<Mesh*> Meshes;
    fmat4 Transform;
    
    Object(const aiNode* node, Model* model);
    
    json_t* CreateJSON();
};


class Material
{
public:
    unsigned Index;
    string Name;
    string ShadingModel;
    string DiffuseTextureName;
    string NormalTextureName;
    string SpecularTextureName;
    
    fvec4 AmbientColor;
    fvec4 DiffuseColor;
    fvec4 SpecularColor;
    
    float AmbientCoeff;
    float DiffuseCoeff;
    float SpecularPower;
    float FresnelPower;
    float Roughness;
    
    Material(const aiMaterial* material, Model* model);
    
    json_t* CreateJSON();
};





#endif
