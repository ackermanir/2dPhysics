#include "triangle.hpp"

#include <string>
#include <iostream>
#include <fstream>
#include <gtx/rotate_vector.hpp>

/* Triangle definitions. */
Triangle::Triangle(glm::vec2 &v0, glm::vec2 &v1, glm::vec2 &v2) {
    init(v0, v1, v2);
}

//Width is the length from center to any corner
Triangle::Triangle(glm::vec2 &center, float width) {
    //height = 3/2 * width. Side = sqrt(3) * width
    float sqR = 1.7320508075688772935274463415058f;
    glm::vec2 top = glm::vec2(center[0], center[1] + width);
    glm::vec2 left = glm::vec2(center[0] - (sqR * width / 2.0f),
                               center[1] - (width / 2.0f));
    glm::vec2 right = glm::vec2(center[0] + (sqR * width / 2.0f),
                                center[1] - (width / 2.0f));
    //want counterclockwise for openGL standards
    init(top, left, right);
}

void Triangle::init(glm::vec2 &v0, glm::vec2 &v1, glm::vec2 &v2) {
    verts[0] = v0;
    verts[1] = v1;
    verts[2] = v2;

    velocity = glm::vec2(.0f, .0f);
    rotationalVelocity = .0f;
	glArraySetup();
}

//project v2 on v1
glm::vec2 project(glm::vec2 v1, glm::vec2 v2) {
    return glm::dot(v2, glm::normalize(v1)) * glm::normalize(v1);
}

void doIt(glm::vec2* verts, int dim,
          glm::vec2 &min, glm::vec2 &max) {
    for (int i = 1; i < 3; i++) {
        if (min[dim] > verts[i][dim]) {
            min[dim] = verts[i][dim];
        }
        if (max[dim] < verts[i][dim]) {
            max[dim] = verts[i][dim];
        }
    }
}

bool Triangle::aabbCollision(Triangle &other) {
    glm::vec2 minA = verts[0];
    glm::vec2 maxA = verts[0];
    doIt(verts, 0, minA, maxA);
    doIt(verts, 1, minA, maxA);
    glm::vec2 minB = other.verts[0];
    glm::vec2 maxB = other.verts[0];
    doIt(other.verts, 0, minB, maxB);
    doIt(other.verts, 1, minB, maxB);

	//Fail fast
	if (minA[0] > maxB[0] ||
		minA[1] > maxB[1] ||
		maxA[0] < minB[0] ||
		maxA[1] < minB[1]) {
		return false;
	}
	return true;
}


/*
  Returns true if there is a collision between objects using SAT
  collision detection.  If there is a collision, colVec will contain
  the minimal vector to resolve collision.  colVec is from A to B
  (i.e. B-A). A is this, B is other.

  There is no information of where the collision occurs in this algo.
 */
bool Triangle::isCollision(Triangle &other, glm::vec2 &colVec) {
    if (! aabbCollision(other)) { return false; }
    const glm::vec2 * ptsA = verts;
    const glm::vec2 * ptsB = other.verts;
    glm::vec2 midA = midPt();
    glm::vec2 midB = other.midPt();
    glm::vec2 norms[6];
    //All 6 norms of the triangles, first 3 are from outer triangle, last are from inner triangle
    for (int i = 0; i < 3; i++) {
        norms[i] = project(ptsB[(i + 1) % 3] - ptsB[i], midB - ptsB[i]);
        norms[i] = glm::normalize(norms[i] - (midB - ptsB[i]));

        norms[i+3] = project(ptsA[(i + 1) % 3] - ptsA[i], midA - ptsA[i]);
        norms[i+3] = glm::normalize(norms[i+3] - (midA - ptsA[i]));
    }
    float minDepth = FLT_MAX;
    for (int j = 0; j < 6; j++) {
        glm::vec2 normal = norms[j];
        float minA = FLT_MAX;
        float maxA = -FLT_MAX;
        float minB = FLT_MAX;
        float maxB = -FLT_MAX;
        for (int i = 0; i < 3; i++) {
            float projDist = glm::dot(ptsA[i], normal);
            if (projDist < minA) { minA = projDist;}
            if (projDist > maxA) { maxA = projDist;}
            projDist = glm::dot(ptsB[i], normal);
            if (projDist < minB) { minB = projDist;}
            if (projDist > maxB) { maxB = projDist;}
        }

        // If we find some gap, return false immeditely
        if ((minA > maxB) || (maxA < minB)) {
            return false;
        } else {
            //Needs fixing
            if ((maxA > minB && minB > minA) && (maxA - minB < minDepth)) {
                colVec = norms[j] * -1.0f * (maxA - minB);
                minDepth = maxA - minB;
            } else if ((minA < maxB && minA > minB) && (maxB - minA < minDepth)) {
                colVec = norms[j] * (maxB - minA);
                minDepth = maxB - minA;
            }
            if (glm::dot(colVec, (midB - midA)) < 0.0f) {
                colVec *= -1.0f;
            }
        }
    }
    return true;
}


// Clips v1 and v2 into pts based on whether they are further than cutoff along norm
// returns the number of points kept stored in pts
int clip(glm::vec2 v1, glm::vec2 v2, glm::vec2 norm,
         float cutoff, glm::vec2 *pts) {
    int kept = 0;
    float dist1 = glm::dot(norm, v1) - cutoff;
    float dist2 = glm::dot(norm, v2) - cutoff;
    if (dist1 > 0.0f) {
        pts[kept] = v1;
        kept++;
    }
    if (dist2 > 0.0f) {
        pts[kept] = v2;
        kept++;
    }
    if (dist1 * dist2 < 0.0f) {
        glm::vec2 dif = v2 - v1;
        float percLeft = dist1 / (dist1 - dist2);
        dif *= percLeft;
        dif += v1;
        pts[kept] = dif;
        kept++;
    }
    return kept;
}

// selects an edge from this that is most perpindicular to norm
// chosenPt is edge furthest along norm
// Two pts used stored inside edge
void Triangle::bestEdge(glm::vec2 norm, glm::vec2 &chosenPt, glm::vec2 *edge) {
    float max = -FLT_MAX;
    int index = 0;
    //Make index the pt furthest into other shape
    for (int i = 0; i < 3; i++) {
        float proj = glm::dot(norm, verts[i]);
        if (proj > max) {
            max = proj;
            index = i;
        }
    }
    chosenPt = verts[index];
    glm::vec2 v0 = verts[(index + 2) % 3];
    glm::vec2 v1 = verts[(index + 1) % 3];
    glm::vec2 cw = chosenPt - v0;
    glm::vec2 ccw = chosenPt - v1;
    // Want the edge most perpendicular
    if (glm::dot(cw, norm) < glm::dot(ccw, norm)) {
        edge[0] = v0;
        edge[1] = chosenPt;
    } else {
        edge[0] = chosenPt;
        edge[1] = v1;
    }
}


// Uses clipping to determine the collision point between triangle A and B
// given the SAT discovered minimal translation vector colVec
bool Triangle::findCollisionPt(Triangle &other,
                               glm::vec2 colVec, glm::vec2 &colPt) {
    const glm::vec2 * ptsA = verts;
    const glm::vec2 * ptsB = other.verts;
    glm::vec2 colNorm = glm::normalize(colVec);
    glm::vec2 edgeA[2];
    glm::vec2 ptA;
    glm::vec2 edgeB[2];
    glm::vec2 ptB;
    bestEdge(colNorm, ptA, edgeA);
    other.bestEdge(-1.0f * colNorm, ptB, edgeB);

    glm::vec2 ref[2];
    glm::vec2 inc[2];
    glm::vec2 refPt;
    bool flip = false;
    float edgeAD =
        glm::dot(ptA - (ptA == edgeA[0] ? edgeA[1] : edgeA[0]), colNorm);
    float edgeBD =
        glm::dot(ptB - (ptB == edgeB[0] ? edgeB[1] : edgeB[0]), colNorm);
    if (edgeAD < edgeBD) {
        refPt = ptA;
        ref[0] = edgeA[0];
        ref[1] = edgeA[1];
        inc[0] = edgeB[0];
        inc[1] = edgeB[1];
    } else {
        refPt = ptB;
        ref[0] = edgeB[0];
        ref[1] = edgeB[1];
        inc[0] = edgeA[0];
        inc[1] = edgeA[1];
        flip = true;
    }

    glm::vec2 clipN1 = glm::normalize(ref[1] - ref[0]);
    glm::vec2 clipped[2];
    int kept = clip(inc[0], inc[1], clipN1,
                    glm::dot(ref[0], clipN1), clipped);
    if (kept < 2) {
        return false;
    }

    kept = clip(clipped[0], clipped[1], -1.0f * clipN1,
                -1.0f * glm::dot(ref[1], clipN1), clipped);
    if (kept < 2) {
        return false;
    }

    glm::vec2 clipN3 = glm::vec2(clipN1[1], clipN1[0] * -1.0f);
    if (flip) {
        clipN3 *= -1.0f;
    }
    float max = glm::dot(clipN3, refPt);

    float clip0Depth = glm::dot(clipN3, clipped[0]) - max;
    float clip1Depth = glm::dot(clipN3, clipped[1]) - max;
    if (clip0Depth < 0.0f) {
        clipped[0] = clipped[1];
        kept--;
    }

    if (clip1Depth < 0.0f) {
        if (kept == 2) {
            clipped[0] = clipped[1];
        }
        kept--;
    }

    if (kept == 2) {
        colPt = (clipped[0] + clipped[1]) / 2.0f;
        return true;
    } else if (kept == 1) {
        colPt = clipped[0];
        return true;
    }
    return false;
}



/*
  Returns all collisions between this triangle and other. All vectors
  are in form of other - this; i.e. apply force of spring relative to
  distance of vec2 in its direction to tail for this.
 */
bool Triangle::collide(Triangle &other) {
    glm::vec2 colVec;
    if (! isCollision(other, colVec)) {
        return false;
    }
    glm::vec2 colPt;
    if (! findCollisionPt(other, colVec, colPt)) {
        return false;
    }

    //reverse this
    colPt += colVec;
    colVec = -colVec;
    handleCollisions(other, colPt, colVec);
    return true;
}

void capFloat(float& input, float max) {
    if (input > max) {
        input = max;
    } else if (input < -max) {
        input = -max;
    }
}

/*
  Fixes the collision specified by col by applying force to both the triangles
  t1 and t2 according to hookes law.
  t1 is this, t2 is other.
 */
 void Triangle::handleCollisions(Triangle &other, glm::vec2 colPt,
                                 glm::vec2 sprVec) {
    //force is sum total of forces of each spring in cols
    // This is only doing linear force
    glm::vec2 linForce(0.0f, 0.0f);
    float t1RotForce = 0.0f;
    float t2RotForce = 0.0f;

    glm::vec2 force = sprVec * HOOKE_CONSTANT;
    linForce += force;
    //project Force onto radius line
    //subtract of this to get perpindicular force
    glm::vec2 t1Radius = colPt - midPt();
    glm::vec2 t2Radius = colPt - other.midPt();
    glm::vec2 t1PerpForce = force - glm::dot(force, glm::normalize(t1Radius));
    glm::vec2 t2PerpForce = force - glm::dot(force, glm::normalize(t2Radius));

    // CCW checks to see what direction the rotation is in
    // It would be great to replace with something cheaper
    bool ccw = glm::cross(glm::vec3(t1PerpForce, 0.0f),
                          glm::vec3(t1Radius, 0.0f)).z > 0.0f;
    bool ccw2 = glm::cross(glm::vec3(t2PerpForce, 0.0f),
                           glm::vec3(t2Radius, 0.0f)).z > 0.0f;

    t1RotForce +=  (-1.0f + ccw * 2.0f) * glm::length(t1PerpForce)
        / glm::length(t1Radius);
    t2RotForce +=  (-1.0f + ccw2 * 2.0f) * glm::length(t2PerpForce)
        / glm::length(t2Radius);

    capFloat(linForce[0], 1.0f);
    capFloat(linForce[1], 1.0f);
    capFloat(t1RotForce, 0.04f);
    capFloat(t2RotForce, 0.04f);
    //Apply force to both triangles in opposite directions
    // dv = F * 1/m
    velocity += linForce * invMass;
    other.velocity -= linForce * other.invMass;

    rotationalVelocity -= t1RotForce * invMass;
    other.rotationalVelocity -= t2RotForce * other.invMass;
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
    velocity += glm::vec2(.0f, -10.0f) * delta;
    //Simulate air drag
    rotationalVelocity *= 0.9995f;
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

Tri_g createTriG(Triangle &t) {
    Tri_g gt;
    for (int i = 0; i < 3; i++) {
        float * v = (float*)&gt.verts[i];
        v[0] = t.verts[i][0];
        v[1] = t.verts[i][1];
    }
    float *v = (float*)&gt.velocity;
    v[0] = t.velocity[0];
    v[1] = t.velocity[1];
    gt.rot_vel = t.rotationalVelocity;
    gt.inv_mass = t.invMass;
    gt.vert_buf = t.vertexBuf;
    return gt;
}
