#pragma once

///// NOTHING IN HERE SHOULD DEPEND ON OTHER FILES /////

// arrays & vectors
template <typename T>
struct myArray {
	std::size_t size;
	T* arr;
};

template <typename T> 
myArray<T> vector_to_array(std::vector<T> vec)
{
	myArray arr = myArray{ vec.size()*sizeof(T), new T[vec.size()] };
    int i = 0;
    for (T element : vec) {
        arr.arr[i++] = element;
    }
	return arr;
}
template <typename T> 
std::vector<T> merge_vectors(std::vector<T> vec1, std::vector<T> vec2)
{
	vec1.insert(vec1.end(), vec2.begin(), vec2.end());
	return vec1;
}
