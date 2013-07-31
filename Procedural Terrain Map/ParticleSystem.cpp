#include "ParticleSystem.h"

#define MAX_VELOCITY 50
#define MAX_LIFETIME 300
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
    velocity = v;
    force = f;
    scale = s;
    
    // Random lifetime up to MAX
    age = 0;
    lifetime = rand(0, MAX_LIFETIME);
    
    // Random orientation
    orientation = fquat();//angleAxis(rand(0, 360), vec3(0, 0, 1)) *
    angleAxis(rand(0, 360), vec3(0, 1, 0)) *
    angleAxis(rand(0, 360), vec3(1, 0, 0));
}


void Particle::Update(float elapsedTime)
{
    location += velocity * elapsedTime;
    velocity *= 0.95;
    velocity += force * elapsedTime;
    scale *= 0.95;
    age += 1;
}

ParticleCluster::ParticleCluster(glm::vec3 location, glm::vec3 c)
{
    color = c;
    
    srand((unsigned int)time(NULL));
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
                           const glm::vec3& cameraPos,  const glm::mat4& viewInverse,
                           GLenum mode)
{
    if (particles.size() == 0)
        return;
    
    p.SetUniform("base_color", color);
    
    vector<vec3> vertices;
	vector<size_t> indices;
    
    // Add triangle for each particle
	for (std::vector<Particle>::iterator it = particles.begin();
         it != particles.end();
         it++)
    {
        Particle particle = *it;
        
        // Orient vertices
        vec3 o1 = particle.orientation * vec3(0.2, 0.0, 0.0);
        vec3 o2 = particle.orientation * vec3(0.0, sqrt(3.0) / 5, 0);
        vec3 o3 = particle.orientation * vec3(-0.2, 0.0, 0.0);
        
        vec3 v1 = particle.location + particle.scale * o1;
        vec3 v2 = particle.location + particle.scale * o2;
        vec3 v3 = particle.location + particle.scale * o3;
        
        glBegin(GL_TRIANGLES);
        glVertex3f(v1.x, v1.y, v1.z);
        glVertex3f(v2.x, v2.y, v2.z);
        glVertex3f(v3.x, v3.y, v3.z);
        glEnd();
    }
}

void FluidCluster::Update(float elapsedTime)
{
    ParticleCluster::Update(elapsedTime);
    while (particles.size() < 1000) {
        vec3 velocity(0, rand(0, MAX_VELOCITY), 0);
        normalize(velocity);
        
        float scale = (float)rand() / RAND_MAX;
        
        AddParticle(location, velocity, wind, scale);
    }
}

FluidCluster::FluidCluster(glm::vec3 l, glm::vec3 w, glm::vec3 c)
{
    location = l;
    wind = w;
    color = c;
    
    srand((unsigned int)time(NULL));
    for (int i = 0; i < PARTICLES_PER_CLUSTER; i++)
    {
        vec3 velocity(0, rand(0, MAX_VELOCITY), 0);
        normalize(velocity);
        
        float scale = (float)rand() / RAND_MAX;
        
        AddParticle(location, velocity, wind, scale);
    }
}

// Generates and draws particle buffer
void FluidCluster::Draw(const Program& p, const glm::mat4& viewProjection,
                        const glm::vec3& cameraPos, const glm::mat4& viewInverse,
                        GLenum mode)
{
    if (particles.size() == 0)
        return;
    
    if (!particleTexture) {
        particleTexture = new Texture("particletexture.bmp");
    }
    
    p.SetUniform("base_color", color);
    p.SetUniform("textured", 1);
    p.SetUniform("texture", particleTexture, GL_TEXTURE0);
    quat camQuat = quat_cast(viewInverse);
    
    vector<vec3> vertices;
	vector<size_t> indices;
    
    glDepthMask(GL_FALSE);
    
    // Add triangle for each particle
    int count = 0;
	for (std::vector<Particle>::iterator it = particles.begin();
         it != particles.end();
         it++)
    {
        Particle particle = *it;
        
        // Orient vertices
        vec3 o1 = particle.orientation * vec3(0.2, -0.2, 0.0);
        vec3 o2 = particle.orientation * vec3(0.2, 0.2, 0);
        vec3 o3 = particle.orientation * vec3(-0.2, 0.2, 0.0);
        vec3 o4 = particle.orientation * vec3(-0.2, -0.2, 0.0);
        
        vec3 v1 = particle.location + particle.scale * o1 * camQuat;
        vec3 v2 = particle.location + particle.scale * o2 * camQuat;
        vec3 v3 = particle.location + particle.scale * o3 * camQuat;
        vec3 v4 = particle.location + particle.scale * o4 * camQuat;
        
        glBegin(GL_TRIANGLE_FAN);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(v1.x, v1.y, v1.z);
        glTexCoord2f(1.0, 1.0);
        glVertex3f(v2.x, v2.y, v2.z);
        glTexCoord2f(0.0, 1.0);
        glVertex3f(v3.x, v3.y, v3.z);
        glTexCoord2f(0.0, 0.0);
        glVertex3f(v4.x, v4.y, v4.x);
        glEnd();
        
        count++;
    }
    
    glDepthMask(GL_TRUE);
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
          const glm::vec3& cameraPos, const glm::mat4& viewInverse, GLenum mode)
{
    for (std::vector<ParticleCluster *>::iterator it = clusters.begin();
         it != clusters.end();
         it++)
    {
        ParticleCluster *cluster = *it;
        cluster->Draw(p, viewProjection, cameraPos, viewInverse, mode);
    }
}