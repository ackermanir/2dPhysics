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


// Tests if there is a collision of these two lines.
// Math formula taken from wikipedia's line-line intersection page.
// Sets none to true if no intersection.
// Returns the intersection point if there is one.
glm::vec2 Triangle::lineCollision(const glm::vec2 &a1, const glm::vec2 &a2,
                                  const glm::vec2 &b1, const glm::vec2 &b2, bool &none) {
    float denom = (a1.x - a2.x)*(b1.y - b1.y) - (a1.y - a2.y)*(b1.x-b2.x);
    if (denom < 0.0001f && denom > -0.0001f) {
        none = true;
        return glm::vec2(0.0f, 0.0f);
    }
    float xNum = (a1.x*a2.y - a1.y*a2.x)*(b1.x - b2.x) -
        (a1.x - a2.x)*(b1.x*b2.y - b1.y*b2.x);
    float yNum = (a1.x*a2.y - a1.y*a2.x)*(b1.y - b2.y) -
        (a1.y - a2.y)*(b1.x*b2.y - b1.y*b2.x);
    xNum /= denom;
    yNum /= denom;

    //line collision must take place within the line segments
    if (!(((xNum > a1.x && xNum < a2.x) || (xNum < a1.x && xNum > a2.x)) &&
          ((xNum > b1.x && xNum < b2.x) || (xNum < b1.x && xNum > b2.x)) &&
          ((yNum > a1.y && yNum < a2.y) || (yNum < a1.y && yNum > a2.y)) &&
          ((yNum > b1.y && yNum < b2.y) || (yNum < b1.y && yNum > b2.y)))) {
        none = true;
        return glm::vec2(0.0f, 0.0f);
    }
    return glm::vec2(xNum, yNum);
}

//If sides are colliding of these triangles, adds a collision to cols
void Triangle::sidesColliding(Triangle *innerT, Triangle *outerT, Collision *cols) {
    const glm::vec2 * outerPts = outerT->verts;
    const glm::vec2 * innerPts = innerT->verts;
    float distance = -1.0f;
    glm::vec2 chosenPt;
    glm::vec2 colPt;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            bool errors = false;
            glm::vec2 col =
                lineCollision(innerPts[i], innerPts[(i + 1) % 3],
                              outerPts[j], outerPts[(j + 1) % 3], errors);
            if (errors) { continue; }
            glm::vec2 pts[4] = {innerPts[i],
                                innerPts[(i + 1) % 3],
                                outerPts[j],
                                outerPts[(j + 1) % 3]};
			float dists[4] = {glm::length(col - pts[0]),
                             glm::length(col - pts[1]),
                             glm::length(col - pts[2]),
                             glm::length(col - pts[3])};
            float distMin = dists[0];
            glm::vec2 minPt = pts[0];
            for (int i = 1; i < 4; i++) {
                if (distMin > dists[i]) {
                    distMin = dists[i];
                    chosenPt = pts[i];
                }
            }
			if (distMin > distance) {
                chosenPt = minPt;
                distance = distMin;
                colPt = col;
            }
        }
    }
    if (distance != -1.0f) {
        cols->addCollision(chosenPt, chosenPt - colPt);
    }
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

void capFloat(float& input, float max) {
    if (input > max) {
        input = max;
    } else if (input < -max) {
        input = -max;
    }
}


//project v2 on v1
glm::vec2 project(glm::vec2 v1, glm::vec2 v2) {
    return glm::dot(v2, glm::normalize(v1)) * glm::normalize(v1);
}

/*
  Returns true if there is  a collision between objects using SAT collision detection.
  If there is a collision, colVec will contain the minimal vector to resolve collision.
  colVec is from A to B (i.e. B-A)
  There is no information of where the collision occurs in this algo.
 */
bool Triangle::isCollision(Triangle *triA, Triangle *triB, glm::vec2 &colVec) {
    const glm::vec2 * ptsA = triA->verts;
    const glm::vec2 * ptsB = triB->verts;
    glm::vec2 midA = triA->midPt();
    glm::vec2 midB = triB->midPt();
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
bool Triangle::findCollisionPt(Triangle *triA, Triangle *triB, glm::vec2 colVec, glm::vec2 &colPt) {
    const glm::vec2 * ptsA = triA->verts;
    const glm::vec2 * ptsB = triB->verts;
    glm::vec2 colNorm = glm::normalize(colVec);
    glm::vec2 edgeA[2];
    glm::vec2 ptA;
    glm::vec2 edgeB[2];
    glm::vec2 ptB;
    triA->bestEdge(colNorm, ptA, edgeA);
    triB->bestEdge(-1.0f * colNorm, ptB, edgeB);

    glm::vec2 ref[2];
    glm::vec2 inc[2];
    glm::vec2 refPt;
    bool flip = false;
    float edgeAD = glm::dot(ptA - (ptA == edgeA[0] ? edgeA[1] : edgeA[0]), colNorm);
    float edgeBD = glm::dot(ptB - (ptB == edgeB[0] ? edgeB[1] : edgeB[0]), colNorm);
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

  Returned Collision must be deleted!
 */
Collision * Triangle::testColliding(Triangle *other) {
    glm::vec2 colVec;
    if (! Triangle::isCollision(this, other, colVec)) {
        return new Collision(this, other);
    }
    glm::vec2 colPt;
    if (! Triangle::findCollisionPt(this, other, colVec, colPt)) {
        return new Collision(this, other);
    }
    // Collision * cols = ptsColliding(this, other);
    Collision * cols = new Collision(other, this);
    cols->addCollision(colPt, colVec);
    return cols;
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
        bool ccw2 = glm::cross(glm::vec3(t2PerpForce, 0.0f), glm::vec3(t2Radius, 0.0f)).z > 0.0f;
        t1RotForce +=  (-1.0f + ccw * 2.0f) * glm::length(t1PerpForce) / glm::length(t1Radius);
        t2RotForce +=  (-1.0f + ccw2 * 2.0f) * glm::length(t2PerpForce) / glm::length(t2Radius);
    }

    capFloat(linForce[0], 1.0f);
    capFloat(linForce[1], 1.0f);
    capFloat(t1RotForce, 0.4f);
    capFloat(t2RotForce, 0.4f);
    //Apply force to both triangles in opposite directions
    // dv = F * 1/m
    col->t1->velocity += linForce * col->t1->invMass;
    col->t2->velocity -= linForce * col->t2->invMass;
    // last multiplier is due to fact that moment of inertia isn't precise
    col->t1->rotationalVelocity -= t1RotForce * col->t1->invMass;
    col->t2->rotationalVelocity -= t2RotForce * col->t2->invMass;
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
