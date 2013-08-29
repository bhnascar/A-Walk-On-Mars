#include "ParticleSystem.h"

#define MAX_VELOCITY 50
#define MAX_LIFETIME 3.0f
#define MAX_TURBULENCE 0.1f
#define PARTICLES_PER_CLUSTER 80

using namespace::std;
using namespace::glm;

static Texture *particleTexture;

float rand(float min, float max) {
    return min + (float)rand() / RAND_MAX * (max - min);
}

Particle::Particle(glm::vec3 l, glm::vec3 v, glm::vec3 f, float s)
{
    location = l;
    velocity = v / 100.0f;
    force = f;
    scale = 1.0f;
    
    // Random lifetime up to MAX
    age = rand(0, MAX_LIFETIME / 2);
    lifetime = rand(0, MAX_LIFETIME);
    
    // Identity orientation
    orientation = fquat();
}


void Particle::Update(float elapsedTime)
{
    location += velocity * elapsedTime;
    velocity += force * elapsedTime;
    velocity += vec3(rand(-MAX_TURBULENCE, MAX_TURBULENCE),
                     rand(0, MAX_TURBULENCE),
                     rand(-MAX_TURBULENCE, MAX_TURBULENCE));
    scale *= 1.03;
    age += elapsedTime;
}

ParticleCluster::ParticleCluster(glm::vec3 location, glm::vec3 c)
{
    model = NULL;
    color = c;
    
    for (int i = 0; i < PARTICLES_PER_CLUSTER; i++)
    {
        vec3 velocity(rand(-MAX_VELOCITY, MAX_VELOCITY),
                      rand(-MAX_VELOCITY, MAX_VELOCITY),
                      rand(-MAX_VELOCITY, MAX_VELOCITY));
        normalize(velocity);
        
        float scale = (float)rand() / RAND_MAX;
        
        AddParticle(location, velocity, vec3(0, -MAX_VELOCITY, 0), scale);
    }
}

void ParticleCluster::AddParticle(glm::vec3 location, glm::vec3 velocity, glm::vec3 force, float scale)
{
    particles.push_back(Particle(location, velocity, force, scale));
}

void ParticleCluster::Update(float elapsedTime)
{
    for (int i = 0; i < particles.size(); i++) {
        Particle &particle = particles[i];
        if (!particle.Valid())
        {
            particles.erase(particles.begin() + i);
            i--;
        }
        else {
            particle.Update(elapsedTime);
        }
    }
}

// Generates and draws particle buffer
void ParticleCluster::Draw(const Program& p, const glm::mat4& viewProjection,
                           const glm::vec3& cameraPos,  const glm::quat& cameraQuat,
                           GLenum mode)
{
    if (particles.size() == 0)
        return;
    
    p.SetUniform("base_color", color);
    
    // pre allocate space for vectors
    vector<vec3> vertices(particles.size() * 3);
    
    // add triangle for each particle
    for (size_t i = 0; i < particles.size(); i++) {
        Particle &particle = particles[i];
        
        // Orient vertices
        vec3 o1 = particle.orientation * vec3(0.2, 0.0, 0.0);
        vec3 o2 = particle.orientation * vec3(0.0, sqrt(3.0) / 5, 0);
        vec3 o3 = particle.orientation * vec3(-0.2, 0.0, 0.0);
        
        vertices[i * 3] = particle.location + particle.scale * o1;
        vertices[i * 3 + 1] = particle.location + particle.scale * o2;
        vertices[i * 3 + 2] = particle.location + particle.scale * o3;
    }
    
    ArrayBuffer<vec3> ab(vertices);
    
    if (model)
        delete model;
    model = new Model(ModelBuffer(ab, (int)vertices.size() / 3), Material(), Bounds());
    
    p.SetModel(mat4(1)); // Needed for Phong shading
    p.SetMVP(viewProjection);
    
    p.SetUniform("base_color", color);
    glLineWidth(3.0);
    model->Draw(p, GL_TRIANGLES);
}

void FluidCluster::Update(float elapsedTime)
{
    ParticleCluster::Update(elapsedTime);
    while (particles.size() < 500) {
        vec3 velocity(0, rand(0, MAX_VELOCITY), 0);
        normalize(velocity);
        
        float scale = (float)rand() / RAND_MAX;
        
        AddParticle(location + vec3(rand(-1, 1), 0, 0), velocity, wind, scale);
    }
}

FluidCluster::FluidCluster(glm::vec3 l, glm::vec3 w, glm::vec3 c)
{
    location = l;
    wind = w;
    color = c;
    
    for (int i = 0; i < PARTICLES_PER_CLUSTER; i++)
    {
        vec3 velocity(0, rand(0, MAX_VELOCITY), 0);
        velocity /= MAX_VELOCITY;
        
        AddParticle(location + vec3(rand(-1, 1), 0, 0), velocity, wind, 1.0f);
    }
}

// Generates and draws particle buffer
void FluidCluster::Draw(const Program& p, const glm::mat4& viewProjection,
                        const glm::vec3& cameraPos, const glm::quat& cameraQuat,
                        GLenum mode)
{
    if (particles.size() == 0)
        return;
    
    if (!particleTexture) {
        particleTexture = new Texture("Textures/particletexture.bmp");
    }
    
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    
    // pre allocate space for vectors
    vector<vec3> vertices(particles.size() * 4);
    vector<vec2> texCoords(particles.size() * 4);
    vector<vec3> age(particles.size() * 4);
    
    // Add triangle for each particle
    int count = 0;
	for (size_t i = 0; i < particles.size(); i++) {
        Particle &particle = particles[i];
        
        // Orient vertices
        vec3 o1 = vec3(0.2, -0.2, 0.0);
        vec3 o2 = vec3(0.2, 0.2, 0);
        vec3 o3 = vec3(-0.2, 0.2, 0.0);
        vec3 o4 = vec3(-0.2, -0.2, 0.0);
        
        vertices[i * 4] = particle.location + cameraQuat * o1 * particle.scale;
        vertices[i * 4 + 1] = particle.location + cameraQuat * o2 * particle.scale;
        vertices[i * 4 + 2] = particle.location + cameraQuat * o3 * particle.scale;
        vertices[i * 4 + 3] =  particle.location + cameraQuat * o4 * particle.scale;
        
        texCoords[i * 4] = vec2(1.0, 0.0);
        texCoords[i * 4 + 1] = vec2(1.0, 1.0);
        texCoords[i * 4 + 2] = vec2(0.0, 1.0);
        texCoords[i * 4 + 3] = vec2(0.0, 0.0);
        
        age[i * 3] = vec3(particle.age, 0, 0);
        age[i * 3 + 1] = vec3(particle.age, 0, 0);
        age[i * 3 + 2] = vec3(particle.age, 0, 0);
        age[i * 3 + 3] = vec3(particle.age, 0, 0);
        
        count++;
    }
    
    ArrayBuffer<vec3> ab(vertices);
    ArrayBuffer<vec2> tb(texCoords);
    ArrayBuffer<vec3> ageBuffer(age);
    model = new Model(ModelBuffer(ab, tb, (int)vertices.size() / 3), Material(), Bounds());
    
    ageBuffer.Use(p, "particleAge");
    
    p.SetUniform("textured", 1);
    p.SetUniform("max_age", MAX_LIFETIME);
    p.SetUniform("texture", particleTexture, GL_TEXTURE0);
    p.SetUniform("base_color", color);
    
    model->Draw(p, GL_QUADS);
    
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    
    cout << count << " particles drawn" << endl;
    
    p.SetUniform("textured", 0);
}

void ParticleSystem::Update(float elapsedTime)
{
	for (int i = 0; i < clusters.size(); i++ ) {
		ParticleCluster *cluster = clusters[i];
		if (!cluster->Valid()) {
            delete cluster;
			clusters.erase(clusters.begin() + i);
			i--;
		} else {
			cluster->Update(elapsedTime - lastTime);
		}
	}
    lastTime = elapsedTime;
}

void ParticleSystem::AddExplosionCluster(glm::vec3 location, glm::vec3 color)
{
    ParticleCluster *cluster = new ParticleCluster(location, color);
    clusters.push_back(cluster);
}

void ParticleSystem::AddFluidCluster(glm::vec3 location, glm::vec3 wind, glm::vec3 color)
{
    FluidCluster *cluster = new FluidCluster(location, wind, color);
    clusters.push_back(cluster);
}

void ParticleSystem::Draw(const Program& p, const glm::mat4& viewProjection,
                          const glm::vec3& cameraPos, const glm::quat& cameraQuat, GLenum mode)
{
    for (std::vector<ParticleCluster *>::iterator it = clusters.begin();
         it != clusters.end();
         it++)
    {
        ParticleCluster *cluster = *it;
        cluster->Draw(p, viewProjection, cameraPos, cameraQuat, mode);
    }
}