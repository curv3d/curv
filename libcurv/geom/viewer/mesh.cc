#include "mesh.h"

#include <iostream>
#include <fstream> 

//#include "fs.h"
#include "geom.h"
#include "text.h"
#include "vertexLayout.h"

//#include "tinyobjloader/tiny_obj_loader.h"

Mesh::Mesh():m_drawMode(GL_TRIANGLES) {

}

Mesh::Mesh(const Mesh &_mother):m_drawMode(_mother.getDrawMode()) {
    add(_mother);
}

Mesh::~Mesh(){

}

void Mesh::setDrawMode(GLenum _drawMode) {
    m_drawMode = _drawMode;
}

void Mesh::setColor(const glm::vec4 &_color) {
    m_colors.clear();
    for (uint i = 0; i < m_vertices.size(); i++) {
        m_colors.push_back(_color);
    }
}

void Mesh::addColor(const glm::vec4 &_color) {
    m_colors.push_back(_color);
}

void Mesh::addColors(const std::vector<glm::vec4> &_colors) {
    m_colors.insert(m_colors.end(), _colors.begin(), _colors.end());
}

void Mesh::addVertex(const glm::vec3 &_point){
   m_vertices.push_back(_point);
}

void Mesh::addVertices(const std::vector<glm::vec3>& _verts){
   m_vertices.insert(m_vertices.end(),_verts.begin(),_verts.end());
}

void Mesh::addVertices(const glm::vec3* verts, int amt){
   m_vertices.insert(m_vertices.end(),verts,verts+amt);
}

void Mesh::addNormal(const glm::vec3 &_normal){
    m_normals.push_back(_normal);
}

void Mesh::addNormals(const std::vector<glm::vec3> &_normals ){
    m_normals.insert(m_normals.end(), _normals.begin(), _normals.end());
}

void Mesh::addTexCoord(const glm::vec2 &_uv){
    m_texCoords.push_back(_uv);
}

void Mesh::addTexCoords(const std::vector<glm::vec2> &_uvs){
    m_texCoords.insert(m_texCoords.end(), _uvs.begin(), _uvs.end());
}

void Mesh::addIndex(uint16_t _i){
    m_indices.push_back(_i);
}

void Mesh::addIndices(const std::vector<uint16_t>& inds){
	m_indices.insert(m_indices.end(),inds.begin(),inds.end());
}

void Mesh::addIndices(const uint16_t* inds, int amt){
	m_indices.insert(m_indices.end(),inds,inds+amt);
}

void Mesh::addTriangle(uint16_t index1, uint16_t index2, uint16_t index3){
    addIndex(index1);
    addIndex(index2);
    addIndex(index3);
}

void Mesh::add(const Mesh &_mesh){

    if(_mesh.getDrawMode() != m_drawMode){
        std::cout << "INCOMPATIBLE DRAW MODES" << std::endl;
        return;
    }

    uint16_t indexOffset = (uint16_t)getVertices().size();

    addColors(_mesh.getColors());
    addVertices(_mesh.getVertices());
    addNormals(_mesh.getNormals());
    addTexCoords(_mesh.getTexCoords());

    for (uint i = 0; i < _mesh.getIndices().size(); i++) {
        addIndex(indexOffset+_mesh.getIndices()[i]);
    }
}

GLenum Mesh::getDrawMode() const{
    return m_drawMode;
}

const std::vector<glm::vec4> & Mesh::getColors() const{
    return m_colors;
}

const std::vector<glm::vec3> & Mesh::getVertices() const{
	return m_vertices;
}

const std::vector<glm::vec3> & Mesh::getNormals() const{
    return m_normals;
}

const std::vector<glm::vec2> & Mesh::getTexCoords() const{
    return m_texCoords;
}

const std::vector<uint16_t> & Mesh::getIndices() const{
    return m_indices;
}

std::vector<glm::ivec3> Mesh::getTriangles() const {
    std::vector<glm::ivec3> faces;

    if(getDrawMode() == GL_TRIANGLES) {
        if(m_indices.size()>0){
            for(unsigned int j = 0; j < m_indices.size(); j += 3) {
                glm::ivec3 tri;
                for(int k = 0; k < 3; k++) {
                    tri[k] = m_indices[j+k];
                }
                faces.push_back(tri);
            }
        } else {
            for( unsigned int j = 0; j < m_vertices.size(); j += 3) {
                glm::ivec3 tri;
                for(int k = 0; k < 3; k++) {
                    tri[k] = j+k;
                }
                faces.push_back(tri);
            }
        }
    } else {
        //  TODO
        //
        std::cout << "ERROR: Mesh only add GL_TRIANGLES for NOW !!" << std::endl;
    }

    return faces;
}

void Mesh::clear(){
    if(!m_vertices.empty()){
		m_vertices.clear();
	}
	if(!m_colors.empty()){
		m_colors.clear();
	}
	if(!m_normals.empty()){
		m_normals.clear();
	}
    if(!m_indices.empty()){
		m_indices.clear();
	}
}

void Mesh::computeNormals(){

    if(getDrawMode() == GL_TRIANGLES){
        //The number of the vertices
        int nV = m_vertices.size();

        //The number of the triangles
        int nT = m_indices.size() / 3;

        std::vector<glm::vec3> norm( nV ); //Array for the normals

        //Scan all the triangles. For each triangle add its
        //normal to norm's vectors of triangle's vertices
        for (int t=0; t<nT; t++) {

            //Get indices of the triangle t
            int i1 = m_indices[ 3 * t ];
            int i2 = m_indices[ 3 * t + 1 ];
            int i3 = m_indices[ 3 * t + 2 ];

            //Get vertices of the triangle
            const glm::vec3 &v1 = m_vertices[ i1 ];
            const glm::vec3 &v2 = m_vertices[ i2 ];
            const glm::vec3 &v3 = m_vertices[ i3 ];

            //Compute the triangle's normal
            glm::vec3 dir = glm::normalize(glm::cross(v2-v1,v3-v1));

            //Accumulate it to norm array for i1, i2, i3
            norm[ i1 ] += dir;
            norm[ i2 ] += dir;
            norm[ i3 ] += dir;
        }

        //Normalize the normal's length and add it.
        m_normals.clear();
        for (int i=0; i<nV; i++) {
            addNormal( glm::normalize(norm[i]) );
        }

    } else {
        //  TODO
        //
        std::cout << "ERROR: Mesh only add GL_TRIANGLES for NOW !!" << std::endl;
    }
}

Vbo* Mesh::getVbo() {

    // Create Vertex Layout
    //
    std::vector<VertexLayout::VertexAttrib> attribs;
    attribs.push_back({"position", 3, GL_FLOAT, POSITION_ATTRIBUTE, false, 0});
    int  nBits = 3;

    bool bColor = false;
    if (getColors().size() > 0 && getColors().size() == m_vertices.size()){
        attribs.push_back({"color", 4, GL_FLOAT, COLOR_ATTRIBUTE, false, 0});
        bColor = true;
        nBits += 4;
    }

    bool bNormals = false;
    if (getNormals().size() > 0 && getNormals().size() == m_vertices.size()){
        attribs.push_back({"normal", 3, GL_FLOAT, NORMAL_ATTRIBUTE, false, 0});
        bNormals = true;
        nBits += 3;
    }

    bool bTexCoords = false;
    if (getTexCoords().size() > 0 && getTexCoords().size() == m_vertices.size()){
        attribs.push_back({"texcoord", 2, GL_FLOAT, TEXCOORD_ATTRIBUTE, false, 0});
        bTexCoords = true;
        nBits += 2;
    }

    VertexLayout* vertexLayout = new VertexLayout(attribs);
    Vbo* tmpMesh = new Vbo(vertexLayout);
    tmpMesh->setDrawMode(getDrawMode());

    std::vector<GLfloat> data;
    for(uint i = 0; i < m_vertices.size(); i++){
        data.push_back(m_vertices[i].x);
        data.push_back(m_vertices[i].y);
        data.push_back(m_vertices[i].z);
        if(bColor){
            data.push_back(m_colors[i].r);
            data.push_back(m_colors[i].g);
            data.push_back(m_colors[i].b);
            data.push_back(m_colors[i].a);
        }
        if(bNormals){
            data.push_back(m_normals[i].x);
            data.push_back(m_normals[i].y);
            data.push_back(m_normals[i].z);
        }
        if(bTexCoords){
            data.push_back(m_texCoords[i].x);
            data.push_back(m_texCoords[i].y);
        }
    }

    tmpMesh->addVertices((GLbyte*)data.data(), m_vertices.size());

    if(getIndices().size()==0){
        if ( getDrawMode() == GL_LINES ) {
            for (uint i = 0; i < getVertices().size(); i++){
                addIndex(i);
            }
        } else if ( getDrawMode() == GL_LINE_STRIP ) {
            for (uint i = 1; i < getVertices().size(); i++){
                addIndex(i-1);
                addIndex(i);
            }
        }
    }

    tmpMesh->addIndices(m_indices.data(), m_indices.size());

    return tmpMesh;
}
