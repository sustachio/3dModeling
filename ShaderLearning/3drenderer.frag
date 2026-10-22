#version 460 core
uniform vec2 uResolution;
uniform float uTime;
out vec4 FragColor;

vec2 fragCoord = gl_FragCoord.xy;

#define MAX_MARCH_STEPS 200
#define MAX_MARCH_DIST 25.
#define MARCH_NEAR_DIST .01

layout(std430, binding = 1) buffer SSBO
{
    float data[];
};

struct closestObject {
    float d;
    vec3 col;
};

///// SDFs ////

float sphereSDF(vec3 p, float radius)
{
    return length(p) - radius;
}

float planeSDF(vec3 p)
{
    return abs(p.y);
    //return p.y;
}

float doughnutSDF( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xz)-t.x,p.y);
  return length(q)-t.y;
}

// courtesy of https://iquilezles.org/articles/distfunctions/
float cubeSDF( vec3 p, vec3 b )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

///// transformations /////

vec3 translate(vec3 p, vec3 pos)
{
    return p-pos;
}

vec3 scale(vec3 p, vec3 sca)
{
    return p/sca;
}

vec3 rotate(vec3 p, vec3 rot)
{
    rot = mod(rot, 360.)/(180./3.1415); // degrees to radians
    
    float ca = cos(rot.x); float sa = sin(rot.x);
    float cb = cos(rot.y); float sb = sin(rot.y);
    float cy = cos(rot.z); float sy = sin(rot.z);
    
    return p*mat3(
        cb*cy         , cb*sy         , -sb,
        sa*sb*cy-ca*sy, sa*sb*sy+ca*cy, sa*cb,
        ca*sb*cy+sa*sy, ca*sb*sy-sa*cy, ca*cb
    );
}

///// Operations /////

closestObject unionObject(closestObject a, closestObject b)
{
    if (a.d < b.d)
        return a;
    return b;
}

closestObject intersectionSDF(closestObject a, closestObject b)
{
    if (a.d > b.d)
        return a;
    return b;
}

closestObject differenceSDF(closestObject b, closestObject a) {
    if (-a.d > b.d)
        return closestObject(-a.d,a.col);
    return b;
}

closestObject smoothUnionObject( closestObject a, closestObject b, float k ) 
{
    float h = clamp( 0.5 + 0.5*(b.d-a.d)/k, 0.0, 1.0 );
    return closestObject(mix(b.d,a.d,h)-k*h*(1.0-h), mix(b.col,a.col,h)); 
}

/////////// LOAD OBJECTS IN FROM OBJECT LIST INTO SINGLE SDF ////////////

const int objectsLength = 100;
const int operatorsLength = 100;

// https://replit.com/@sustachi0/Render-Object-Shader-Inputs-Attempt-2#main.py
// these should be ints, but all data is passed in as a series of floats
// types
const float OPERATOR = 0.;
const float OBJECT = 1.;
// operators
const float DOWN = 0.;
const float UNION = 1.;
const float SMOOTH_UNION = 2.;
const float INTERSECTION_UNION = 3.;
const float DIFFERENCE_UNION = 4.;
// state
const float NEXT_ITEM = 0.;
const float NEXT_POINT_PARAMETER = 1.;
const float OBJECT_NAME = 2.;
const float NEXT_SDF_PARAMETER = 3.;
///// sdfs
const float PLANE = 0.;
const float CUBE = 1.;
const float SPHERE = 2.;
///// point components
const float POINT_END = 0.;
const float POSITION = 1.;
const float ROTATION = 2.;
///// post-sdf components
const float SDF_END = 0.;
const float COLOR = 1.;

/*
float[] objects = float[](
   OPERATOR, UNION,
    OBJECT, POSITION, 0., 0., .9, POINT_END, PLANE, COLOR, 1., 0., 0., SDF_END,
    OBJECT, POSITION, 0., -.4, 0., POINT_END, SPHERE, COLOR, 1., 1., 0., SDF_END,
    OPERATOR, UNION,
        OBJECT, POSITION, .5, 0., .95, POINT_END, SPHERE, COLOR, 1., 0., 1., SDF_END,
        OBJECT, POSITION, -.5, 0., -.95, POINT_END, SPHERE, COLOR, 0., 1., 1., SDF_END,
    	OPERATOR, DOWN,
    OPERATOR, DOWN
);
*/

closestObject apply_operations(closestObject value1, closestObject value2, float op)
{
    if (op == UNION)
        return unionObject(value1, value2);
    if (op == SMOOTH_UNION)
        return smoothUnionObject(value1, value2, 1.);
    if (op == INTERSECTION_UNION)
        return intersectionSDF(value1, value2);
    if (op == DIFFERENCE_UNION)
        return differenceSDF(value1, value2);

    return closestObject(0., vec3(1,1,1));
}

/*
closestObject getClosestObject2(vec3 p)
{
    closestObject ground = closestObject(
        planeSDF(p), 
        vec3(0,1,1));
        
    closestObject wall = closestObject(
        abs(p.z-15.), 
        vec3(1,1,1));
        
    closestObject sphere1 = closestObject(
        sphereSDF(
            p - vec3(-2.,1,15.),
            2),
        vec3(1,0,0));
        
    closestObject sphere2 = closestObject(
        sphereSDF(
            p - vec3(0,1,15.),
            2),
        vec3(0,1,0));
        
    closestObject sphere3 = closestObject(
        sphereSDF(
            p - vec3(2.,1,15.),
            2),
        vec3(0,0,1));
    

    closestObject scene = unionObject(ground, sphere1);
    scene = unionObject(scene, sphere2);
    scene = unionObject(scene, sphere3);
    scene = unionObject(scene, wall);

    return scene;
}
*/

closestObject getClosestObject(vec3 p)
{
    float state = NEXT_ITEM;

    closestObject objectBuilder = closestObject(0.,vec3(1,0,1));
    vec3 objectP = p;
    
    float currentOperator = -1.;
    // fake variable length lists
    closestObject[objectsLength] valueStack;    int valueStackIndex = -1;
    float[operatorsLength]       operatorStack; int operatorStackIndex = -1;

    for (int i=0; i<data.length(); i++)
    {
        //############ NEXT_ITEM ##########

        // object
        // NEXT_ITEM -> NEXT_POINT_PARAMETER
        if (state == NEXT_ITEM && data[i] == OBJECT)
        {
        	objectBuilder = closestObject(0.,vec3(1,0,0));
            objectP = p;
            
            state = NEXT_POINT_PARAMETER;
            continue;
        }

        // down
        // NEXT_ITEM -> NEXT_ITEM
        if (state == NEXT_ITEM && data[i] == OPERATOR && data[i+1] == DOWN)
        {
            operatorStackIndex = max(operatorStackIndex-1, -1); // pop

            if (operatorStackIndex > -1)
                currentOperator = operatorStack[operatorStackIndex];

            if (valueStackIndex > 0) // two or more items
            {
                closestObject val2 = valueStack[valueStackIndex];
                closestObject val1 = valueStack[valueStackIndex-1];
                valueStackIndex -= 2;
                         
                valueStackIndex += 1;
                valueStack[valueStackIndex] = apply_operations(val1, val2, currentOperator);
            }

            state = NEXT_ITEM;
            i += 1; // skip operator
            continue;
        }
        
        // normal operation
        // NEXT_ITEM -> NEXT_ITEM
        if (state == NEXT_ITEM && data[i] == OPERATOR)
        {
            // don't forget not yet used operators when changing operators
            if ((currentOperator != -1.) && (
                    (operatorStackIndex == -1) || 
                    (operatorStackIndex > -1 && operatorStack[operatorStackIndex] != currentOperator)))
            {
                operatorStackIndex += 1;
                operatorStack[operatorStackIndex] = currentOperator;
            }
            
            currentOperator = data[i+1];
            
            state = NEXT_ITEM;
            i += 1; // skip operator
            continue;
        }

        //############ NEXT_POINT_PARAMETER #########
        
        // load point parameters
        // NEXT_POINT_PARAMETER -> NEXT_POINT_PARAMETER
        if (state == NEXT_POINT_PARAMETER && data[i] != POINT_END)
        {
            //----------- POINT PARAMETER DEFINITIONS -----------//
            if (data[i] == POSITION)
            {
                objectP -= vec3(data[i+1], data[i+2], data[i+3]);
                i += 3;
            }
            if (data[i] == ROTATION)
            {
                objectP = rotate(objectP, vec3(data[i+1], data[i+2], data[i+3]));
                i += 3;
            }
            //---------------------------------------------------//
            continue;
        }

        // break
        // NEXT_POINT_PARAMETER -> OBJECT_NAME
        if (state == NEXT_POINT_PARAMETER && data[i] == POINT_END)
        {
            state = OBJECT_NAME;
            continue;
        }

        //############ OBJECT_NAME (SDF) ############
        
        // OBJECT_NAME -> NEXT_SDF_PARAMETER
        if (state == OBJECT_NAME)
        {
            float dist = 0.;
            //---------- SDFS DEFINITIONS -----------#
            if (data[i] == PLANE)
            {
                dist = planeSDF(objectP);
			}
            if (data[i] == CUBE)
            {
                dist = cubeSDF(objectP, vec3(data[i+1], data[i+2], data[i+3]));
                i += 3;
			}
            if (data[i] == SPHERE)
            {
            	dist = sphereSDF(objectP, data[i+1]);
                i += 1;
			}
            //---------------------------------------#
            
            objectBuilder = closestObject(dist, vec3(1,0,0));
            
            state = NEXT_SDF_PARAMETER;
            continue;
        }

        //######### NEXT_SDF_PARAMETER #######

        // load sdf parameters
        // NEXT_SDF_PARAMETER -> NEXT_SDF_PARAMETER
        if (state == NEXT_SDF_PARAMETER && data[i] != SDF_END)
        {
            //----------- POST-SDF PARAMETERS -----------#
            if (data[i] == COLOR)
            {
                objectBuilder.col = vec3(data[i+1], data[i+2], data[i+3]);
                i += 3;
            }
            //------------------------------------------#

            continue;
        }

        // move to next object
        // NEXT_SDF_PARAMETER -> NEXT_ITEM
        if (state == NEXT_SDF_PARAMETER && data[i] == SDF_END)
        {
            valueStackIndex += 1;
            valueStack[valueStackIndex] = objectBuilder;

            // apply operation if still in same mode
            if (operatorStackIndex > -1 && operatorStack[operatorStackIndex] == currentOperator)
            {
                closestObject val2 = valueStack[valueStackIndex];
                closestObject val1 = valueStack[valueStackIndex-1];
                valueStackIndex -= 2;
                
                operatorStackIndex -= 1;
                         
                valueStackIndex += 1;
                valueStack[valueStackIndex] = apply_operations(val1, val2, currentOperator);
            }

            operatorStackIndex += 1;
            operatorStack[operatorStackIndex] = currentOperator;
            
            state = NEXT_ITEM;

            continue;
        }
    }

	// clear stacks just to be sure
	//for (int i=1; i<valueStack.length(); i++)
		//valueStack[i] = closestObject(0., vec3(1,0,1));
	//for (int i=0; i<operatorStack.length(); i++)
		//operatorStack[i] = 99.;

    return valueStack[0];
}

vec3 getNormal(vec3 p)
{
    vec2 e = vec2(.01,0);
     
    vec3 n1 = vec3(
        getClosestObject(p+e.xyy).d,
        getClosestObject(p+e.yxy).d,
        getClosestObject(p+e.yyx).d
    );
    vec3 n2=vec3(
        getClosestObject(p-e.xyy).d,
        getClosestObject(p-e.yxy).d,
        getClosestObject(p-e.yyx).d
    );
         
    return normalize(n1-n2);
}

float rayMarch(vec3 rayOrigin, vec3 rayDirection)
{
    float distanceFromOrigin = 0.;

    for (int step=0; step<MAX_MARCH_STEPS && distanceFromOrigin<MAX_MARCH_DIST; step++)
    {
	    vec3 point = rayOrigin+rayDirection*distanceFromOrigin;

		float d = getClosestObject(point).d;
        distanceFromOrigin += d;
        
        // hit something 
        if (d <= MARCH_NEAR_DIST)
            return distanceFromOrigin;
    }

	// if it didn't hit anything
    return MAX_MARCH_DIST;
}


vec3 getLighting(vec3 p)
{
	vec3 col = getClosestObject(p).col;

    vec3 lightRay = normalize(vec3(1,1,-.5));

    float lighting = clamp(dot(lightRay, getNormal(p)), 0., 1.)+.5;   
    //lighting = floor(lighting*5.)/5.;

    return col*lighting;
}

vec4 mainImage()
{
    // normalize pixel cordinates
    vec2 uv=(gl_FragCoord.xy-.5*uResolution.xy)/uResolution.y;

    vec3 camera = vec3(0., 1, 0.);
    vec3 rayDirection = normalize(vec3(uv.x,uv.y,1));
    
   	float d = rayMarch(camera, rayDirection);
    vec3 hitP = camera+rayDirection*d;
    
    vec3 col = getLighting(hitP);
    	
    //return vec4(rayMarchErrorCheck(camera, rayDirection), 1.);

    //return vec4(vec3(d), 1.0);
    //return vec4(vec3(getClosestObject(p).col), 1.0);
    //return vec4(vec3(step(0., getClosestObject(camera+rayDirection).d)), 1.0);
    return vec4(col*vec3(1.-d/MAX_MARCH_DIST),1.0);
}

void main()
{
    FragColor = mainImage();
}
