#include <iostream>

#include <vector>
#include <list>
#include <variant>
#include <string>
#include <format>

#include "generic_utils.h"

#pragma once

enum item_tyoe
{
	Operator,
	Object,
};

const int UNION_OP_COUNT = 5;
enum joining_operator
{
	Down, // go back to last operation, used in nesting
	Union,
	Smooth_Union,
	Intersection_Union,
	Difference_Union,
};


enum point_parameters
{
	Point_End, // control
	Position,
	Rotation,
};

const int SDF_TYPES_COUNT = 3;
enum sdf_types
{
	Plane,
	Cube,
	Sphere,
};

enum post_sdf_parameters
{
	SDF_End, // control
	Color,
};

////// INTERNAL //////

enum states
{
	Next_Item,
	Next_Point_Parameter,
	Object_Name,
	Next_SDF_Parameter,
};

//////////////////////////


struct point_parameter {
	int type;
	std::vector<float> args;
};

struct post_sdf_parameter {
	int type;
	std::vector<float> args;
};

struct group;
struct object;
struct item;

struct object {
	int sdf;
	std::vector<float> sdfParameters;
	std::vector<point_parameter> pointParameters;
	std::vector<post_sdf_parameter> postSDFParameters;
};

struct group {
	int operator_;
	std::vector<item*> children;
};

// item in heirchy
int items = 0;
struct item {
	int id; // needed to search for in a vector
	std::string name;
	item* parent;
	std::variant<group, object> data;

	item(std::string name_, std::variant<group, object> data_) { 
		data = data_; 
		name = name_;
		parent = nullptr;
		id = items++;

		// set childrens' parents to this
		if (isGroup())
		{
			for (item* child : getGroup().children)
			{
				child->parent = this;

				printf(child->to_string().c_str());
			}
		}
	};

	bool operator ==(item b) // needed for finding in children list
	{
		return id == b.id;
	}

	object& getObject() { return std::get<object>(data); }
	group&  getGroup()  { return std::get<group> (data);  }

	bool isObject() { return std::holds_alternative<object>(data); }
	bool isGroup()  { return std::holds_alternative<group> (data); }

	// create a readable string
	std::string to_string(int indentLevel = 0)
	{
		std::string res(indentLevel, ' ');

		// if I have a parent, get its name
		std::string parentName = "NULL";
		if (parent != nullptr)
			parentName = name;

		if (isObject())
			res.append(std::format("{} ({})\n", name, parentName));

		if (isGroup())
		{
			group g = getGroup();

			res.append(std::format("{} ({}):\n", name, parentName));

			indentLevel++;
			for (item* child : g.children)
				res.append(child->to_string(indentLevel));
			indentLevel--;
		}
		
		return res;
	}

	// delete all values being pointed to
	void freeMemory() {
		if (isGroup()) {
			for (item* child : getGroup().children) {
				child->freeMemory();
			}
		}

		delete this;
	}
};

float demoObjects[] = {
	Operator, Union,
	Object, Point_End, Plane, Color, 0, 1, 1, SDF_End,
	Object, Position, -2, 1, 15, Point_End, Sphere, Color, 1, 0, 0, SDF_End,
	Object, Position, 0, 1, 15, Point_End, Sphere, Color, 0, 1, 0, SDF_End,
	Object, Position, 2, 1, 15, Point_End, Sphere, Color, 0, 0, 1, SDF_End,
	Operator, Down,
};


std::vector<float> group_to_float_inputs(item* itemInput)
{
	group input = itemInput->getGroup();

	std::vector<float> result;

	result.push_back(Operator);
	result.push_back(input.operator_);

	for (item* child : input.children)
	{
		// object
		if (child->isObject())
		{
			object obj = child->getObject();

			result.push_back(Object);

			// point parameters
			for (point_parameter param : obj.pointParameters)
			{
				result.push_back(param.type);
				
				for (float arg : param.args)
					result.push_back(arg);
			}
			result.push_back(Point_End);

			// object sdf
			result.push_back(obj.sdf);
			for (float param : obj.sdfParameters)
			{
				result.push_back(param);
			}

			// post sdf parameters
			for (post_sdf_parameter param : obj.postSDFParameters)
			{
				result.push_back(param.type);
				result = merge_vectors(result, param.args);
			}
			result.push_back(SDF_End);
		}

		// group (recursive)
		if (child->isGroup())
			result = merge_vectors(result, 
				group_to_float_inputs(child)
			);
	}

	result.push_back(Operator);
	result.push_back(Down);

	return result;
}
