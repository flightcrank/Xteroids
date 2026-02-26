
#ifndef PLY_H
#define PLY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"

//the max size in bytes this parser can read from a .ply file
#define MAX_LINE 256

static inline void load_ply(Mesh3D *mesh, char filename[]) {

	FILE *fptr;			//File pointer
	char buffer[MAX_LINE];		//Temporary storage for each line
	int count = 0;			//Int that keeps track of number of lines read
	bool end_header = false;	//Boolean that keeps track of the point the end_header has been reached in the .ply file
	long position = 0;		//File position to jump to for a second pass

	// Open file for reading ("r")
	fptr = fopen(filename, "r");

	if (fptr == NULL) {
		
		printf("Error: Could not open file.\n");
	}

	//First pass: read file line by line
	while (fgets(buffer, MAX_LINE, fptr)) {
		
		//search for the line starting with "element vertex" and save the number of vertices described. Allocate memory on the heap for each vertex in the mesh
		if (strstr(buffer, "element vertex")) {
			
			sscanf(buffer, "%*s %*s %d", &mesh->local_count);
			mesh->local_verts = malloc(mesh->local_count * sizeof(Vector3));
			//model->screen_verts = malloc(model->local_count * sizeof(Vector3));
			
			if (mesh->local_verts == NULL) {
				
				puts("Could not allocate memory for vert data");
				free(mesh->local_verts);
				//free(model->screen_verts);
				return;
			}
		}

		//search for the line starting with "element face" and save the number of vertices that make up each face. Allocate memory on the heap to save this data to an array
		if (strstr(buffer, "element face")) {
			
			sscanf(buffer, "%*s %*s %d", &mesh->facev_count);
			mesh->facev = malloc(mesh->facev_count * sizeof(int));	
			
			if (mesh->facev == NULL) {
				
				puts("Could not allocate memory for face data");
				return;
			}
		}
		
		//search line for the string "end header" set a boolean indicating it has been found and continue to read the next line
		if (strstr(buffer, "end_header")) {
			
			end_header = true;
			continue;
		}

		//vert then face data begins
		if (end_header) {
			
			//read vert data and store in an array of Vector3 structs on the heap
			if (count < mesh->local_count) {
				
				//Save position in file where the last vert position is found and the face data begins
				if (count == mesh->local_count - 1) {
					
					position = ftell(fptr);
				}
				
				sscanf(buffer, "%f %f %f", &mesh->local_verts[count].x, &mesh->local_verts[count].y, &mesh->local_verts[count].z);
				count++;

			//read the face data and populate an array that stores the number of vertices per face. also keep track of how many vertex indices are needed to define every face in the mesh
			} else if (count >= mesh->local_count) {
				
				int index = count - mesh->local_count;
				sscanf(buffer, "%i", &mesh->facev[index]);
				mesh->meshf_count += mesh->facev[index];
				count++;

			} else {
				
				break;
			}
		}
	}
	
	//int to keep track of the faces_array index we are storing every vertex index for ever face in the mesh
	int index = 0;				
	
	//Allocate memory on the heap for the large array containing all the indices into the local_verts array for every face in the mesh
	mesh->meshf = malloc(mesh->meshf_count * sizeof(int));
	
	if (mesh->meshf == NULL) {
		
		printf("Could not allocate memory for all the faces in the mesh object ( meshf_count = %d)", mesh->meshf_count);
		return;	
	}
	
	//position the file offset at the position save during the first pass at the beginning of the first line of mesh face data
	fseek(fptr, position, SEEK_SET);
	
	//Second pass: save face data in array
	//loop through the lines in the file 
	for (int i = 0; i < mesh->facev_count; i++) {
		
		fgets(buffer, MAX_LINE, fptr);	//read the line from the file which represents a single face in the mesh

		char *start = buffer;		//pointer to point to the current number in the line buffer to read
		char *next;			//pointer to point to the next number in the line buffer to read

		strtol(start, &next, 10);	//read first number to skip over it and set *next to the next number in the line buffer(which is the number of vertices for this face to get to the actuall vertex index data)
		start = next;			//set the new start point in the line buffer

		//read "N" number of indices and save them to an array
		for (int j = 0; j < mesh->facev[i]; j++) {
	
			int value = (int) strtol(start, &next, 10);
			mesh->meshf[index] = value;		//save value in the faces array where all vertex indices are saved for every face in the mesh
			start = next;					//set new start point in the line buffer
			index++;					//increment to index into the faces_array
		}
	}

	fclose(fptr); // Always close the file
}

static inline void model3D_free(Mesh3D *mesh) {
	
	if (mesh == NULL) {

		return;
	}

	//free the malloc'd vertex, facev and meshf array
	free(mesh->local_verts);
	free(mesh->facev);
	free(mesh->meshf);
}

#endif
