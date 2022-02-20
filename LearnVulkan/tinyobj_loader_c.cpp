#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader_c.h"


void tinyobj_attrib_init(tinyobj_attrib_t* attrib) {
	attrib->vertices = NULL;
	attrib->num_vertices = 0;
	attrib->normals = NULL;
	attrib->num_normals = 0;
	attrib->texcoords = NULL;
	attrib->num_texcoords = 0;
	attrib->faces = NULL;
	attrib->num_faces = 0;
	attrib->face_num_verts = NULL;
	attrib->num_face_num_verts = 0;
	attrib->material_ids = NULL;
}

void tinyobj_attrib_free(tinyobj_attrib_t* attrib) {
    if (attrib->vertices) TINYOBJ_FREE(attrib->vertices);
    if (attrib->normals) TINYOBJ_FREE(attrib->normals);
    if (attrib->texcoords) TINYOBJ_FREE(attrib->texcoords);
    if (attrib->faces) TINYOBJ_FREE(attrib->faces);
    if (attrib->face_num_verts) TINYOBJ_FREE(attrib->face_num_verts);
    if (attrib->material_ids) TINYOBJ_FREE(attrib->material_ids);
}

void tinyobj_shapes_free(tinyobj_shape_t* shapes, size_t num_shapes) {
    size_t i;
    if (shapes == NULL) return;

    for (i = 0; i < num_shapes; i++) {
        if (shapes[i].name) TINYOBJ_FREE(shapes[i].name);
    }

    TINYOBJ_FREE(shapes);
}

void tinyobj_materials_free(tinyobj_material_t* materials,
    size_t num_materials) {
    size_t i;
    if (materials == NULL) return;

    for (i = 0; i < num_materials; i++) {
        if (materials[i].name) TINYOBJ_FREE(materials[i].name);
        if (materials[i].ambient_texname) TINYOBJ_FREE(materials[i].ambient_texname);
        if (materials[i].diffuse_texname) TINYOBJ_FREE(materials[i].diffuse_texname);
        if (materials[i].specular_texname) TINYOBJ_FREE(materials[i].specular_texname);
        if (materials[i].specular_highlight_texname)
            TINYOBJ_FREE(materials[i].specular_highlight_texname);
        if (materials[i].bump_texname) TINYOBJ_FREE(materials[i].bump_texname);
        if (materials[i].displacement_texname)
            TINYOBJ_FREE(materials[i].displacement_texname);
        if (materials[i].alpha_texname) TINYOBJ_FREE(materials[i].alpha_texname);
    }

    TINYOBJ_FREE(materials);
}

int tinyobj_parse_obj(tinyobj_attrib_t* attrib, tinyobj_shape_t** shapes,
    size_t* num_shapes, tinyobj_material_t** materials_out,
    size_t* num_materials_out, const char* file_name, file_reader_callback file_reader,
    unsigned int flags) {
    LineInfo* line_infos = NULL;
    Command* commands = NULL;
    size_t num_lines = 0;

    size_t num_v = 0;
    size_t num_vn = 0;
    size_t num_vt = 0;
    size_t num_f = 0;
    size_t num_faces = 0;

    int mtllib_line_index = -1;

    tinyobj_material_t* materials = NULL;
    size_t num_materials = 0;

    hash_table_t material_table;

    char* buf = NULL;
    size_t len = 0;
    file_reader(file_name, &buf, &len);

    if (len < 1) return TINYOBJ_ERROR_INVALID_PARAMETER;
    if (attrib == NULL) return TINYOBJ_ERROR_INVALID_PARAMETER;
    if (shapes == NULL) return TINYOBJ_ERROR_INVALID_PARAMETER;
    if (num_shapes == NULL) return TINYOBJ_ERROR_INVALID_PARAMETER;
    if (buf == NULL) return TINYOBJ_ERROR_INVALID_PARAMETER;
    if (materials_out == NULL) return TINYOBJ_ERROR_INVALID_PARAMETER;
    if (num_materials_out == NULL) return TINYOBJ_ERROR_INVALID_PARAMETER;

    tinyobj_attrib_init(attrib);

    /* 1. create line data */
    if (get_line_infos(buf, len, &line_infos, &num_lines) != 0) {
        return TINYOBJ_ERROR_EMPTY;
    }

    commands = (Command*)TINYOBJ_MALLOC(sizeof(Command) * num_lines);

    create_hash_table(HASH_TABLE_DEFAULT_SIZE, &material_table);

    /* 2. parse each line */
    {
        size_t i = 0;
        for (i = 0; i < num_lines; i++) {
            int ret = parseLine(&commands[i], &buf[line_infos[i].pos],
                line_infos[i].len, flags & TINYOBJ_FLAG_TRIANGULATE);
            if (ret) {
                if (commands[i].type == COMMAND_V) {
                    num_v++;
                }
                else if (commands[i].type == COMMAND_VN) {
                    num_vn++;
                }
                else if (commands[i].type == COMMAND_VT) {
                    num_vt++;
                }
                else if (commands[i].type == COMMAND_F) {
                    num_f += commands[i].num_f;
                    num_faces += commands[i].num_f_num_verts;
                }

                if (commands[i].type == COMMAND_MTLLIB) {
                    mtllib_line_index = (int)i;
                }
            }
        }
    }

    /* line_infos are not used anymore. Release memory. */
    if (line_infos) {
        TINYOBJ_FREE(line_infos);
    }

    /* Load material(if exits) */
    if (mtllib_line_index >= 0 && commands[mtllib_line_index].mtllib_name &&
        commands[mtllib_line_index].mtllib_name_len > 0) {
        // char *filename = my_strndup(commands[mtllib_line_index].mtllib_name,
        //                             commands[mtllib_line_index].mtllib_name_len);

        char* updatedFilename = (char*)malloc(strlen(file_name) + 1);
        strcpy_s(updatedFilename, strlen(file_name) + 1, file_name);
        updatedFilename[strlen(file_name) - 3] = 'm';
        updatedFilename[strlen(file_name) - 2] = 't';
        updatedFilename[strlen(file_name) - 1] = 'l';
        int ret = tinyobj_parse_and_index_mtl_file(&materials, &num_materials, updatedFilename, file_reader, &material_table);

        if (ret != TINYOBJ_SUCCESS) {
            /* warning. */
            fprintf(stderr, "TINYOBJ: Failed to parse material file '%s': %d\n", updatedFilename, ret);
        }

        TINYOBJ_FREE(updatedFilename);

    }

    /* Construct attributes */

    {
        size_t v_count = 0;
        size_t n_count = 0;
        size_t t_count = 0;
        size_t f_count = 0;
        size_t face_count = 0;
        int material_id = -1; /* -1 = default unknown material. */
        size_t i = 0;

        attrib->vertices = (float*)TINYOBJ_MALLOC(sizeof(float) * num_v * 3);
        attrib->num_vertices = (unsigned int)num_v;
        attrib->normals = (float*)TINYOBJ_MALLOC(sizeof(float) * num_vn * 3);
        attrib->num_normals = (unsigned int)num_vn;
        attrib->texcoords = (float*)TINYOBJ_MALLOC(sizeof(float) * num_vt * 2);
        attrib->num_texcoords = (unsigned int)num_vt;
        attrib->faces = (tinyobj_vertex_index_t*)TINYOBJ_MALLOC(
            sizeof(tinyobj_vertex_index_t) * num_f);
        attrib->num_faces = (unsigned int)num_f;
        attrib->face_num_verts = (int*)TINYOBJ_MALLOC(sizeof(int) * num_faces);
        attrib->material_ids = (int*)TINYOBJ_MALLOC(sizeof(int) * num_faces);
        attrib->num_face_num_verts = (unsigned int)num_faces;

        for (i = 0; i < num_lines; i++) {
            if (commands[i].type == COMMAND_EMPTY) {
                continue;
            }
            else if (commands[i].type == COMMAND_USEMTL) {
                /* @todo
                   if (commands[t][i].material_name &&
                   commands[t][i].material_name_len > 0) {
                   std::string material_name(commands[t][i].material_name,
                   commands[t][i].material_name_len);

                   if (material_map.find(material_name) != material_map.end()) {
                   material_id = material_map[material_name];
                   } else {
                // Assign invalid material ID
                material_id = -1;
                }
                }
                */
                if (commands[i].material_name &&
                    commands[i].material_name_len > 0)
                {
                    /* Create a null terminated string */
                    char* material_name_null_term = (char*)TINYOBJ_MALLOC(commands[i].material_name_len + 1);
                    memcpy((void*)material_name_null_term, (const void*)commands[i].material_name, commands[i].material_name_len);
                    material_name_null_term[commands[i].material_name_len] = 0;

                    if (hash_table_exists(material_name_null_term, &material_table))
                        material_id = (int)hash_table_get(material_name_null_term, &material_table);
                    else
                        material_id = -1;

                    TINYOBJ_FREE(material_name_null_term);
                }
            }
            else if (commands[i].type == COMMAND_V) {
                attrib->vertices[3 * v_count + 0] = commands[i].vx;
                attrib->vertices[3 * v_count + 1] = commands[i].vy;
                attrib->vertices[3 * v_count + 2] = commands[i].vz;
                v_count++;
            }
            else if (commands[i].type == COMMAND_VN) {
                attrib->normals[3 * n_count + 0] = commands[i].nx;
                attrib->normals[3 * n_count + 1] = commands[i].ny;
                attrib->normals[3 * n_count + 2] = commands[i].nz;
                n_count++;
            }
            else if (commands[i].type == COMMAND_VT) {
                attrib->texcoords[2 * t_count + 0] = commands[i].tx;
                attrib->texcoords[2 * t_count + 1] = commands[i].ty;
                t_count++;
            }
            else if (commands[i].type == COMMAND_F) {
                size_t k = 0;
                for (k = 0; k < commands[i].num_f; k++) {
                    tinyobj_vertex_index_t vi = commands[i].f[k];
                    int v_idx = fixIndex(vi.v_idx, v_count);
                    int vn_idx = fixIndex(vi.vn_idx, n_count);
                    int vt_idx = fixIndex(vi.vt_idx, t_count);
                    attrib->faces[f_count + k].v_idx = v_idx;
                    attrib->faces[f_count + k].vn_idx = vn_idx;
                    attrib->faces[f_count + k].vt_idx = vt_idx;
                }

                for (k = 0; k < commands[i].num_f_num_verts; k++) {
                    attrib->material_ids[face_count + k] = material_id;
                    attrib->face_num_verts[face_count + k] = commands[i].f_num_verts[k];
                }

                f_count += commands[i].num_f;
                face_count += commands[i].num_f_num_verts;
            }
        }
    }

    /* 5. Construct shape information. */
    {
        unsigned int face_count = 0;
        size_t i = 0;
        size_t n = 0;
        size_t shape_idx = 0;

        const char* shape_name = NULL;
        unsigned int shape_name_len = 0;
        const char* prev_shape_name = NULL;
        unsigned int prev_shape_name_len = 0;
        unsigned int prev_shape_face_offset = 0;
        unsigned int prev_face_offset = 0;
        tinyobj_shape_t prev_shape = { NULL, 0, 0 };

        /* Find the number of shapes in .obj */
        for (i = 0; i < num_lines; i++) {
            if (commands[i].type == COMMAND_O || commands[i].type == COMMAND_G) {
                n++;
            }
        }

        /* Allocate array of shapes with maximum possible size(+1 for unnamed
         * group/object).
         * Actual # of shapes found in .obj is determined in the later */
        (*shapes) = (tinyobj_shape_t*)TINYOBJ_MALLOC(sizeof(tinyobj_shape_t) * (n + 1));

        for (i = 0; i < num_lines; i++) {
            if (commands[i].type == COMMAND_O || commands[i].type == COMMAND_G) {
                if (commands[i].type == COMMAND_O) {
                    shape_name = commands[i].object_name;
                    shape_name_len = commands[i].object_name_len;
                }
                else {
                    shape_name = commands[i].group_name;
                    shape_name_len = commands[i].group_name_len;
                }

                if (face_count == 0) {
                    /* 'o' or 'g' appears before any 'f' */
                    prev_shape_name = shape_name;
                    prev_shape_name_len = shape_name_len;
                    prev_shape_face_offset = face_count;
                    prev_face_offset = face_count;
                }
                else {
                    if (shape_idx == 0) {
                        /* 'o' or 'g' after some 'v' lines. */
                        (*shapes)[shape_idx].name = my_strndup(
                            prev_shape_name, prev_shape_name_len); /* may be NULL */
                        (*shapes)[shape_idx].face_offset = prev_shape.face_offset;
                        (*shapes)[shape_idx].length = face_count - prev_face_offset;
                        shape_idx++;

                        prev_face_offset = face_count;

                    }
                    else {
                        if ((face_count - prev_face_offset) > 0) {
                            (*shapes)[shape_idx].name =
                                my_strndup(prev_shape_name, prev_shape_name_len);
                            (*shapes)[shape_idx].face_offset = prev_face_offset;
                            (*shapes)[shape_idx].length = face_count - prev_face_offset;
                            shape_idx++;
                            prev_face_offset = face_count;
                        }
                    }

                    /* Record shape info for succeeding 'o' or 'g' command. */
                    prev_shape_name = shape_name;
                    prev_shape_name_len = shape_name_len;
                    prev_shape_face_offset = face_count;
                }
            }
            if (commands[i].type == COMMAND_F) {
                face_count++;
            }
        }

        if ((face_count - prev_face_offset) > 0) {
            size_t length = face_count - prev_shape_face_offset;
            if (length > 0) {
                (*shapes)[shape_idx].name =
                    my_strndup(prev_shape_name, prev_shape_name_len);
                (*shapes)[shape_idx].face_offset = prev_face_offset;
                (*shapes)[shape_idx].length = face_count - prev_face_offset;
                shape_idx++;
            }
        }
        else {
            /* Guess no 'v' line occurrence after 'o' or 'g', so discards current
             * shape information. */
        }

        (*num_shapes) = shape_idx;
    }

    if (commands) {
        TINYOBJ_FREE(commands);
    }

    destroy_hash_table(&material_table);

    (*materials_out) = materials;
    (*num_materials_out) = num_materials;

    return TINYOBJ_SUCCESS;
}

int tinyobj_parse_mtl_file(tinyobj_material_t** materials_out,
    size_t* num_materials_out,
    const char* filename, file_reader_callback file_reader) {
    return tinyobj_parse_and_index_mtl_file(materials_out, num_materials_out, filename, file_reader, NULL);
}

char* dynamic_fgets(char** buf, size_t* size, FILE* file) {
    char* offset;
    char* ret;
    size_t old_size;

    if (!(ret = fgets(*buf, (int)*size, file))) {
        return ret;
    }

    if (NULL != strchr(*buf, '\n')) {
        return ret;
    }

    do {
        old_size = *size;
        *size *= 2;
        *buf = (char*)TINYOBJ_REALLOC(*buf, *size);
        offset = &((*buf)[old_size - 1]);

        ret = fgets(offset, (int)(old_size + 1), file);
    } while (ret && (NULL == strchr(*buf, '\n')));

    return ret;
}