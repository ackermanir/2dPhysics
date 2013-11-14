#include "triangle.hpp"

#include <string>
#include <iostream>
#include <fstream>
#include <gtx/rotate_vector.hpp>


/* Collision definitions. */
Collision::Collision(Triangle *tr1, Triangle *tr2)
    :cols() {
    t1 = tr1;
    t2 = tr2;
}

/* Reverses other collisions and adds them to this's vector of collisions. */
void Collision::mergeReverse(Collision *other) {
    for (std::vector<std::pair<glm::vec2, glm::vec2>>::iterator it = other->cols.begin();
         it != other->cols.end(); ++it) {
        it->first += it->second;
        it->second = -it->second;
    }
    cols.insert(cols.end(), other->cols.begin(), other->cols.end());
}

void Collision::addCollision(glm::vec2 &point, glm::vec2 &dir) {
    std::pair<glm::vec2, glm::vec2> col(point, dir);
    cols.push_back(col);
}

void Collision::print() {
	std::cout << "Collisions:\n";
    std::cout << "    (collision pt) (spring vector)\n";
    for (unsigned int i = 0; i < cols.size(); i++) {
        std::pair<glm::vec2, glm::vec2> pr = cols[i];
        std::cout << "    (" << pr.first[0] << ", " << pr.first[1] <<
            ") (" << pr.second[0] << ", " << pr.second[1] << ")\n";
    }
}

/* Triangle definitions. */
Triangle::Triangle(glm::vec2 &v0, glm::vec2 &v1, glm::vec2 &v2) {
    init(v0, v1, v2);
}

Triangle::Triangle(glm::vec2 &center, float width) {
    //sqrt(3)/6
    float ratio = 0.28867513459481288225457439025098f;
    glm::vec2 top = glm::vec2(center[0], center[1] + (1.0f - ratio) * width);
    glm::vec2 right = glm::vec2(center[0] + width / 2.0f, center[1] - ratio * width);
    glm::vec2 left = glm::vec2(center[0] - width / 2.0f, center[1] - ratio * width);
    init(top, right, left);
}

void Triangle::init(glm::vec2 &v0, glm::vec2 &v1, glm::vec2 &v2) {
    verts[0] = v0;
    verts[1] = v1;
    verts[2] = v2;

    velocity = glm::vec2(.0f, .0f);
    rotationalVelocity = .0f;
	glArraySetup();
}

/*
  Returns a vector of all vec2s of collisions of points of innerT
  triangle inside the outerT. Each point will either not be inside the
  outer Triangle or will be codified into a vec2 inside collisions of
  the smallest distance from that point to a side.

  Returned vector must be deleted!
 */
Collision *
Triangle::ptsColliding(Triangle *outerT, Triangle *innerT) {
    const glm::vec2 * outerPts = outerT->verts;
    const glm::vec2 * innerPts = innerT->verts;
    Collision * col = new Collision(innerT, outerT);
    glm::vec2 sides[3];
    for (int i = 0; i < 3; i++) {
        //Normalized side of triangle, normalized to allow for projection later
        sides[i] = glm::normalize(outerPts[(i + 1) % 3] - outerPts[i]);
    }
    for (int i = 0; i < 3; i++) {
        glm::vec2 shortest;
        float shortDist = 0.0f;
        for (int j = 0; j < 3; j++) {
            glm::vec2 side(sides[j]);
            glm::vec2 ptFromSide = innerPts[i] - outerPts[j];
            float distAlongSide = glm::dot(side, ptFromSide);
            // Dot product will be positive if point is inside triangle
            // for all sides
            if (distAlongSide < 0.0f) {
                //Not actually inside triangle
                shortDist = 0.0f;
                break;
            }
            //Side point is projection of point on side
            glm::vec2 sidePt = outerPts[j] + side * distAlongSide;
            glm::vec2 diff = sidePt - innerPts[i];
            float maybe = glm::dot(diff, sides[(j + 1) % 3]);
            if (maybe > 0.0f) {
                //Not in triangle, diff to side should point in same directions as
                // nearest two sides
                shortDist = 0.0f;
                break;
            }
            //Should just be dist^2 for speed
            float dist = glm::length(diff);
            if (j == 0 || dist < shortDist) {
                shortest = diff;
                shortDist = dist;
            }
        }
        if (shortDist > 0.0f) {
            //It is colliding, add minimum dist to collisions
            col->addCollision(glm::vec2(innerPts[i]), shortest);
        }
    }
    return col;
}

/*
  Fixes the collision specified by col by applying force to both the triangles
  t1 and t2 according to hookes law.
 */
void Triangle::handleCollisions(Collision * col) {
    //force is sum total of forces of each spring in cols
    // This is only doing linear force
    glm::vec2 linForce(0.0f, 0.0f);
    float t1RotForce = 0.0f;
    float t2RotForce = 0.0f;
    glm::vec2 t1Mid = col->t1->midPt();
    glm::vec2 t2Mid = col->t2->midPt();
    for (unsigned int i = 0; i < col->cols.size(); i++) {
        glm::vec2 force = col->cols[i].second * HOOKE_CONSTANT;
        linForce += force;
        //project Force onto radius line
        //subtract of this to get perpindicular force
        glm::vec2 t1Radius = col->cols[i].first - t1Mid;
        glm::vec2 t2Radius = col->cols[i].first - t2Mid;
        glm::vec2 t1PerpForce = force - glm::dot(force, glm::normalize(t1Radius));
        glm::vec2 t2PerpForce = force - glm::dot(force, glm::normalize(t2Radius));

        //CCW checks to see what direction the rotation is in
        // It would be great to replace with something cheaper
        bool ccw = glm::cross(glm::vec3(t1PerpForce, 0.0f), glm::vec3(t1Radius, 0.0f)).z > 0.0f;
        t1RotForce += (1.0f + ccw * -2.0f) * glm::length(t1PerpForce) * glm::length(t1Radius);
        ccw = glm::cross(glm::vec3(t2PerpForce, 0.0f), glm::vec3(t2Radius, 0.0f)).z > 0.0f;
        t2RotForce += (1.0f + ccw * -2.0f) * glm::length(t2PerpForce) * glm::length(t2Radius);
    }
    //Apply force to both triangles in opposite directions
    // dv = F * 1/m
    col->t1->velocity += linForce * col->t1->invMass;
    col->t2->velocity -= linForce * col->t2->invMass;
    // 10 is magic constant, we are ignoring moment of inertia anyway
    // Really just a factor that sways how much force rotational response should have
    col->t1->rotationalVelocity -= t1RotForce * col->t1->invMass * 10;
    col->t2->rotationalVelocity -= t2RotForce * col->t2->invMass * 10;
}

/*
  Returns all collisions between this triangle and other. All vectors
  are in form of other - this; i.e. apply force of spring relative to
  distance of vec2 in its direction to tail for this.

  Returned Collision must be deleted!
 */
Collision * Triangle::testColliding(Triangle *other) {
    Collision * cols = ptsColliding(other, this);
    if (cols->cols.size() == 0) {
        delete cols;
        cols = ptsColliding(this, other);
    } else {
        Collision * cols2 = ptsColliding(this, other);
        cols->mergeReverse(cols2);
        delete cols2;
    }
    return cols;
}

// Moves the triangle forward in time by delta applying gravity
void Triangle::timeStep(float delta) {
    if (invMass == 0.0f) {
        return;
    }
    glm::vec2 mid = midPt();
    float rotation = rotationalVelocity * delta;
    for (int i = 0 ; i < 3; i++) {
        glm::vec2 rad = verts[i] - mid;
        glm::vec2 rotDelta = glm::rotate(rad, rotation);
        verts[i] = mid + rotDelta;
        verts[i] += velocity * delta;
    }
    velocity += glm::vec2(.0f, -1.0f) * delta;
    //Simulate air drag
    rotationalVelocity *= 0.995f;
    velocity *= 0.9995f;
}

// Determine midpoint of triangle, assumes constant density so simply average
glm::vec2 Triangle::midPt(void) {
    glm::vec2 mid = verts[0] + verts[1] + verts[2];
    mid /= 3.0f;
    return mid;
}


// Setup the VBO for this triangle
void Triangle::glArraySetup(void) {
	//Setup vertex buffer
	glGenBuffers(1, &vertexBuf);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
}

// Update the VBO with the current position of verts
void Triangle::glRedoVBO(void) {
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
}

void Triangle::drawSelf(void) {
	// set up vertices
    glRedoVBO();
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuf);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0,(void*)0);

    glDrawArrays(GL_TRIANGLES, 0, 3);

	glDisableVertexAttribArray(0);
}

void Triangle::print(void) {
	std::cout << "Triangle:\n";
	std::cout << verts[0][0] << " " << verts[0][1] << "\n";
	std::cout << verts[1][0] << " " << verts[1][1] << "\n";
	std::cout << verts[2][0] << " " << verts[2][1] << "\n";
}

Triangle::~Triangle(void) {
	glDeleteBuffers(1, &vertexBuf);
}
