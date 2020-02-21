/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Bryn Murrell $
   ======================================================================== */
#include "blib_utils.h"
#include <malloc.h>
#include <unordered_map>
#include "len_string.h"
#include "parsing.h"
#include "DebugFileIO.cpp"
#include "vulkan/vulkan.h"
#include "shader_binding_info.h"
#include "material_info.h"

flocal VkShaderStageFlagBits
getShaderStage(char* str)
{
    char* path = str;
    while(path[0] != '.')
    {
        ++path;
    }
    ++path;
    if(streq("vert", path, 4))
    {
        return VK_SHADER_STAGE_VERTEX_BIT;
    }
    else 
    {
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    
}
flocal void parseSingleShader(char* path, std::vector<shader_binding_info>* infos, std::vector<vertex_binding_info>* v_infos)
{

    u64 file_len;
    char* buf = read_entire_file_text(path, &file_len);
    VkShaderStageFlagBits stage = getShaderStage(path);

    Tokenizer tokenizer = {};
    tokenizer.at = buf;
    b32 shouldBreak = false;
    while(!shouldBreak)
    {
        Token t = getToken(&tokenizer);
        switch(t.type)
        {
            case TOKEN_END :
            {
                shouldBreak = true;
            }
            break;
            case TOKEN_UNKNOWN :
            {
            }
            break;
            case TOKEN_IDENTIFIER :
            {
                if (tokenEquals(token(TOKEN_IDENTIFIER, 6, "layout"), t))
                {
                    //eat first paren
                    Token paren = getToken(&tokenizer);
                    ASSERT(tokenEquals(token(TOKEN_PAREN_OPEN, 1, "("), paren), "Unexpected token!");
                    Token bindingType = getToken(&tokenizer);
                        
                    if (tokenEquals(token(TOKEN_IDENTIFIER, 13, "push_constant"), bindingType))
                    {
                        shader_binding_info info = {};
                        info.stage_flags = (ShaderStage)stage;
                        info.type = SHADER_DESCRIPTOR_TYPE_MAX_ENUM;
                        info.is_push_constant = true;
                        infos->push_back(info);
                      
                    }
                    else if (tokenEquals(token(TOKEN_IDENTIFIER, 8, "location"), bindingType))
                    {
                        vertex_binding_info info = {};
                        info.stage_flags = (ShaderStage)stage;
                        find_next_token(&tokenizer, token(TOKEN_ASSIGNMENT, 1, "="));
                        Token bindingIndex = getToken(&tokenizer);
                        info.binding_index = lenStringToInt(bindingIndex.text, bindingIndex.length);
                        find_next_token(&tokenizer, token(TOKEN_PAREN_CLOSE, 1, ")"));
                        if (peek_tok(tokenizer) == token(TOKEN_IDENTIFIER, 2, "in"))
                        {
                            getToken(&tokenizer);
                            info.is_shader_input = 1;
                        }
                        else
                        {
                            Token t = getToken(&tokenizer);
                            ASSERT(tokenEquals(t,token(TOKEN_IDENTIFIER, 3, "out")), "Unexpected token!");
                            info.is_shader_input = 0;
                        }
                        Token type_name = getToken(&tokenizer);
                        if (type_name == token(TOKEN_IDENTIFIER, 4, "vec2"))
                        {
                            info.vertex_format = VK_FORMAT_R32G32_SFLOAT;
                        }
                        else if (type_name == token(TOKEN_IDENTIFIER, 4, "vec3"))
                        {
                            info.vertex_format = VK_FORMAT_R32G32B32A32_SFLOAT;
                        }
                        else if (type_name == token(TOKEN_IDENTIFIER, 4, "vec4"))
                        {
                            info.vertex_format = VK_FORMAT_R32G32B32A32_SFLOAT;
                        }
                        else if (type_name == token(TOKEN_IDENTIFIER, 5, "uvec4"))
                        {
                            info.vertex_format = VK_FORMAT_R32G32B32A32_UINT;
                        }
                        v_infos->push_back(info);
                    }
                    else if (tokenEquals(token(TOKEN_IDENTIFIER, 6, "std430"), bindingType))
                    {
                        shader_binding_info info = {};
                        info.stage_flags = (ShaderStage)stage;
                        find_next_token(&tokenizer, token(TOKEN_ASSIGNMENT, 1, "="));
                        Token bindingIndex = getToken(&tokenizer);
                        info.binding = lenStringToInt(bindingIndex.text, bindingIndex.length);
                        
                        ASSERT(tokenEquals(token(TOKEN_PAREN_CLOSE, 1, ")"), getToken(&tokenizer)),
                               "Unexpected token!, %s, %d");
                        
                        Token isBuffer = getToken(&tokenizer);
                        if (tokenEquals(token(TOKEN_IDENTIFIER, 6, "buffer"), isBuffer))
                        {
                            info.type = SHADER_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                        }
                        infos->push_back(info);
                        
                    }
                    else if (tokenEquals(token(TOKEN_IDENTIFIER, 7, "binding"), bindingType))
                    {
                        shader_binding_info info = {};
                        info.stage_flags = (ShaderStage)stage;
                        find_next_token(&tokenizer, token(TOKEN_ASSIGNMENT, 1, "="));
                        Token bindingIndex = getToken(&tokenizer);
                        info.binding = lenStringToInt(bindingIndex.text, bindingIndex.length);
                        ASSERT(tokenEquals(token(TOKEN_PAREN_CLOSE, 1, ")"), getToken(&tokenizer)),
                               "Unexpected token!, %s, %d");
                        getToken(&tokenizer);
                        Token isSampler = getToken(&tokenizer);
                        if (tokenEquals(token(TOKEN_IDENTIFIER, 9, "sampler2D"), isSampler) || tokenEquals(token(TOKEN_IDENTIFIER, 9, "sampler3D"), isSampler))
                        {
                            info.type = SHADER_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        }
                        else
                        {
                            info.type = SHADER_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        }
                        infos->push_back(info);
                    }
                }
            }
            break;
        }
    }
}

flocal inline u32 strLen(char* str)
{
    u32 counter = 0;
    char* copy = str;
    while(*copy)
    {
        counter++;
        copy++;
    }
    return counter;
}

//NOTE(Bryn): VERY BAD!!! WILL LEAD TO MEMORY LEAKS IF RETURN VAL NOT CAPTURED
flocal char*
print_shader_binding_info(const shader_binding_info& bindingInfo, u32* ctr)
{
    char* buf = (char*)malloc(512);
    char* cursor = buf;
    if (bindingInfo.is_push_constant)
    {
        cursor += sprintf(cursor, "Push Constant\n");
    }
    else
    {
        if (bindingInfo.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        {
            cursor += sprintf(cursor, "bindingInfo.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER\n");
        }
        else if (bindingInfo.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        {
            cursor += sprintf(cursor, "bindingInfo.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER\n");
        }
        else if (bindingInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        {
            cursor += sprintf(cursor, "bindingInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER\n");
        }
        if (bindingInfo.stage_flags == VK_SHADER_STAGE_VERTEX_BIT)
        {
            cursor += sprintf(cursor, "bindingInfo.stage_flags == VK_SHADER_STAGE_VERTEX_BIT\n");
        }
        else if (bindingInfo.stage_flags == VK_SHADER_STAGE_FRAGMENT_BIT)
        {
            cursor += sprintf(cursor, "bindingInfo.stage_flags == VK_SHADER_STAGE_FRAGMENT_BIT\n");
        }
        cursor += sprintf(cursor, "bindingInfo.binding == %d\n", bindingInfo.binding);
        
    }
    if (ctr)
    {
        *ctr = cursor - buf;
    }
    return buf;
}

flocal char* print_vertex_binding_info(const vertex_binding_info& info)
{
    len_string l = l_string(512);
    char* cursor = l.str;
    cursor += sprintf(cursor, "Binding index: %d\nIs input: %d\nShader stage: %s\n", info.binding_index, info.is_shader_input, info.stage_flags == SHADER_STAGE_VERTEX_BIT ? "SHADER_STAGE_VERTEX_BIT" : "SHADER_STAGE_FRAGMENT_BIT");
    len_string format = l_string(64);
    switch (info.vertex_format)
    {
        
        case VK_FORMAT_R32G32_SFLOAT :
        {
            append_to_len_string(&format, "VK_FORMAT_R32G32_SFLOAT");
        } break; 
        case VK_FORMAT_R32G32B32A32_SFLOAT :
        {
            append_to_len_string(&format, "VK_FORMAT_R32G32B32A32_SFLOAT");
        } break; 
        case VK_FORMAT_R32G32B32A32_UINT :
        {
            append_to_len_string(&format, "VK_FORMAT_R32G32B32A32_UINT");
        } break;
        default :
        {
            ASSERT(1==0, "Unrecognized vertex format");
        }
    }
    sprintf(cursor, "Vector format %s\n", format.str);
    free_l_string(&format);
    return l.str;
}

char* print_material_info(const material_info& materialInfo, u32* len)
{
    char* buf = (char*)malloc(2048);
    char* cursor = buf;
    cursor += sprintf(cursor, "%.*s\n", materialInfo.vert_shader_path_length, materialInfo.vert_shader_path);
    cursor += sprintf(cursor, "%.*s\n\n", materialInfo.frag_shader_path_length, materialInfo.frag_shader_path);
    for (int i = 0; i < materialInfo.num_bindings; i++)
    {
        cursor += sprintf(cursor, "Binding Info %d:\n%s\n", i, print_shader_binding_info(materialInfo.bindings[i], nullptr));
    }
    LOOP(i, materialInfo.num_vertex_bindings)
    {
        cursor += sprintf(cursor, "Vertex Info %d:\n%s\n", i, print_vertex_binding_info(materialInfo.vertex_bindings[i]));
    }
    if (len)
    {
        *len = cursor - buf;
    }
    return buf;
}

flocal inline char* copyStr(char* a, u32 len)
{
    char* out = (char*)malloc(256);
    memcpy(out, a, len);
    out[len] = 0;
    return out;
}

flocal u32
getCompiledShaderName(char* out, char* name, VkShaderStageFlagBits stage)
{
    u32 outShaderNameLen = strLen(name);
    u32 outLen = outShaderNameLen + 8;
    for (int i = 0; i < outShaderNameLen; i++)
    {
        out[i] = name[i];
    }
    if (stage == VK_SHADER_STAGE_VERTEX_BIT)
    {
        strcat(out, "Vert.spv");
    }
    else
    {
        strcat(out, "Frag.spv");
    }
    return outLen;
}

int
main(u32 argc, char** argv)
{
    ASSERT(argc == 5, "TOO FEW ARGUMENTS SUPPLIED TO COMMAND LINE");
    std::vector<shader_binding_info> infos = {};
    std::vector<vertex_binding_info> v_infos;
    
    
    parseSingleShader(argv[1], &infos, &v_infos);
    parseSingleShader(argv[2], &infos, &v_infos);
    
    material_info matInfo = {};
    matInfo.num_bindings = infos.size();
    matInfo.num_vertex_bindings = v_infos.size();
    
    LOOP(i, matInfo.num_bindings)
    {
        matInfo.bindings[i] = infos[i];
    }
    LOOP(i, matInfo.num_vertex_bindings)
    {
        matInfo.vertex_bindings[i] = v_infos[i];
    }

    matInfo.vert_shader_path_length = getCompiledShaderName((char*)&matInfo.vert_shader_path, argv[4], VK_SHADER_STAGE_VERTEX_BIT);

    
    matInfo.frag_shader_path_length = getCompiledShaderName((char*)&matInfo.frag_shader_path, argv[4], VK_SHADER_STAGE_FRAGMENT_BIT);

    char matPath[256];
    char* ctr = matPath;
    char* inputCtr = argv[3];
    u32 len = 0;
    while (*inputCtr)
    {
        ++len;
        *ctr++ = *inputCtr++;
    }
    *ctr = 0;
    strcat(matPath, ".mat");

    char* logPath = copyStr(matPath, len);
    strcat(logPath, ".log");

    char* compPathVert = copyStr(matPath, len);
    
    FILE* f = fopen(matPath, "wb");
    fwrite(&matInfo, sizeof(matInfo), 1, f);
    fclose(f);
    
    FILE* log = fopen(logPath, "w");
    char* matLog = print_material_info(matInfo, &len);
    
    char vertCommandPath[256];
    sprintf(vertCommandPath,
            "cmd /C W:/VulkanSDK/1.1.130.0/Bin/glslangValidator -V %s -o %s", argv[1], matInfo.vert_shader_path);
    char fragCommandPath[256];
    sprintf(fragCommandPath,
            "cmd /C W:/VulkanSDK/1.1.130.0/Bin/glslangValidator -V %s -o %s", argv[2], matInfo.frag_shader_path);
    
    u32 vert_ret_val = system(vertCommandPath);
    u32 frag_ret_val = system(fragCommandPath);
    
    fwrite(matLog, len, 1, f);
    fclose(log);
    if (vert_ret_val || frag_ret_val)
    {
        printf("\nCompilation failed: Press ENTER key to terminate\n");  
        getchar(); 
    }
    return 0;
}
