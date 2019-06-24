/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Bryn Murrell $
   ======================================================================== */
#include <stdio.h>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <time.h>
#include <stdlib.h>
#include "CommonDefines.h"
#include "VkShaderBindingInfo.h"
#include "DebugFileIO.cpp"
#include <iostream>

enum TokenType
{
    TOKEN_UNKNOWN,
    TOKEN_COMMA,
    TOKEN_EQUALS,
    TOKEN_IDENTIFIER,
    TOKEN_PAREN_OPEN,
    TOKEN_PAREN_CLOSE,
    TOKEN_BRACKET_OPEN,
    TOKEN_BRACKET_CLOSE,
    TOKEN_BRACE_OPEN,
    TOKEN_BRACE_CLOSE,
    TOKEN_NUMBER,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_STRING,
    TOKEN_ASTERISK,
    TOKEN_POUND,
    TOKEN_END
};

struct Token
{
    TokenType type;
    u32 length;
    char* text;
};

struct Tokenizer
{
    char* at;
};

flocal inline b32
isEOL(char c)
{
    return (c == '\n') || (c == '\r');
}


flocal inline b32
isWhitespace(char c)
{
    return ((c == ' ')  || (c == '\t') || isEOL(c));
}

flocal void
eatAllWhitespace(Tokenizer* tok)
{
    while(true)
    {
        if (isWhitespace(tok->at[0]))
        {
            tok->at++;
        }
        else if (tok->at[0] == '/' && tok->at[1] == '/')
        {
            while(tok->at[0] && !isEOL(tok->at[0]))
            {
                tok->at++;
            }
        }
        else if (tok->at[0] == '/' && tok->at[1] == '*')
        {
            while (tok->at[0] && (tok->at[0] != '*' && tok->at[1] != '/'))
            {
                tok->at++;
            }
            if(tok->at[0] != '*')
            {
                tok->at += 2;
            }
        }
        else
        {
            break;
        }
    }
     
}

flocal inline b32
isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

flocal inline b32
isNumeric(char c)
{
    return c >= '0' && c <= '9';   
}

flocal u32
lenStringToInt(char* c, u32 len)
{
    char* buf = (char*)malloc(len + 1);
    for (int i = 0; i < len; i++)
    {
        buf[i] = c[i];
    }
    buf[len] = 0;
    u32 out = strtol(buf, nullptr, 0);
    free(buf);
    return out;
}


Token getToken(Tokenizer* tokenizer)
{
    eatAllWhitespace(tokenizer);
    Token t = {};
    t.length = 1;
    t.text = tokenizer->at;
    switch(tokenizer->at[0])
    {

        case ',' :
        {
            t.type = TOKEN_COMMA;
            tokenizer->at++;
        } break;
        case '=' :
        {
            t.type = TOKEN_EQUALS;
            tokenizer->at++;
        } break;
        case '(' :
        {
            t.type = TOKEN_PAREN_OPEN;
            tokenizer->at++;
        } break;
        case ')' :
        {
            t.type = TOKEN_PAREN_CLOSE; 
            tokenizer->at++;
        } break;
        case '#' :
        {
            t.type = TOKEN_POUND;
            tokenizer->at++;
        } break;
        case '[' :
        {
            t.type = TOKEN_BRACKET_OPEN; 
            tokenizer->at++;
        } break;
        case ']' :
        {
            t.type = TOKEN_BRACKET_CLOSE;
            tokenizer->at++;
        } break;
        case '{' :
        {
            t.type = TOKEN_BRACE_OPEN;
            tokenizer->at++;
        } break;
        case '}' :
        {
            t.type = TOKEN_BRACE_CLOSE; 
            tokenizer->at++;
        } break;
        case ':' :
        {
            t.type = TOKEN_COLON;
            tokenizer->at++;
        } break;
        case ';':
        {
            t.type = TOKEN_SEMICOLON; 
            tokenizer->at++;
        } break;
        case '*':
        {
            t.type = TOKEN_ASTERISK; 
            tokenizer->at++;
        } break;
        case '"' :
        {
            tokenizer->at++;
            while(tokenizer->at[0] && tokenizer->at[0] != '"')
            {
                if (tokenizer->at[0] == '\\' && tokenizer->at[1])
                {
                    tokenizer->at++;
                }
                tokenizer->at++;
            }
            if (tokenizer->at[0] == '"')
            {
                tokenizer->at++;
            }
            t.type = TOKEN_STRING;
            t.length = tokenizer->at - t.text;
        } break;
        case 0 :
        {
            t.type = TOKEN_END;
            t.text = '\0';
            t.length = 0;
        } break;
        default :
        {
            if (isAlpha(tokenizer->at[0]))
            {
                while(isAlpha(tokenizer->at[0]) ||
                      isNumeric(tokenizer->at[0]) ||
                      tokenizer->at[0] == '_')
                {
                    tokenizer->at++;
                }
                t.type = TOKEN_IDENTIFIER;
                t.length = tokenizer->at - t.text;
            }
            else if (isNumeric(tokenizer->at[0]))
            {
                while(!isWhitespace(tokenizer->at[0]) && tokenizer->at[0] != ')')
                {
                    tokenizer->at++;
                }
                t.type = TOKEN_NUMBER;
                t.length = tokenizer->at - t.text;
                
            }
            else
            {
                tokenizer->at++;
                t.type = TOKEN_UNKNOWN;
                t.length = 0;
                t.text = nullptr;
            }
        } break;
        
    }
    return t;
}

flocal b32 strEq(char* a, char* b)
{
    char* c = a;
    char* d = b;
    while (c[0] && d[0])
    {
        if(*c++ != *d++)
        {
            return false;
        }
        
    }
    return true;
}

flocal inline b32
tokenEquals(const Token& a, const Token& b)
{
    return (a.type == b.type && a.length == b.length && strEq(a.text, b.text));
}

flocal inline
Token token(TokenType type, u32 len, char* text)
{
    return {type, len, text};
}

flocal VkShaderStageFlagBits
getShaderStage(char* str)
{
    char* path = str;
    while(path[0] != '.')
    {
        ++path;
    }
    ++path;
    if(strEq("vert", path))
    {
        return VK_SHADER_STAGE_VERTEX_BIT;
    }
    else 
    {
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    
}
flocal void parseSingleShader(char* path, std::vector<ShaderBindingInfo>* infos)
{


    char* buf = readEntireFileText(path);
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
                    ShaderBindingInfo info = {};
                    Token bindingType = getToken(&tokenizer);
                    info.stageFlags = stage;
                        
                    if (tokenEquals(token(TOKEN_IDENTIFIER, 13, "push_constant"), bindingType))
                    {
                        info.type = (VkDescriptorType)-1;
                        info.isPushConstant = true;
                        infos->push_back(info);
                      
                    }
                    else if (tokenEquals(token(TOKEN_IDENTIFIER, 6, "std430"), bindingType))
                    {
                        ASSERT(tokenEquals(token(TOKEN_COMMA, 1, ","), getToken(&tokenizer)),
                               "Unexpected token!");
                        ASSERT(tokenEquals(token(TOKEN_IDENTIFIER, 7, "binding"), getToken(&tokenizer)),
                               "Unexpected token!");
                        ASSERT(tokenEquals(token(TOKEN_EQUALS, 1, "="), getToken(&tokenizer)),
                               "Unexpected token!");
                        Token bindingIndex = getToken(&tokenizer);
                        info.binding = lenStringToInt(bindingIndex.text, bindingIndex.length);
                        
                        ASSERT(tokenEquals(token(TOKEN_PAREN_CLOSE, 1, ")"), getToken(&tokenizer)),
                               "Unexpected token!, %s, %d");
                        
                        Token isBuffer = getToken(&tokenizer);
                        if (tokenEquals(token(TOKEN_IDENTIFIER, 6, "buffer"), isBuffer))
                        {
                            info.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                        }
                        infos->push_back(info);
                        
                    }
                    else if (tokenEquals(token(TOKEN_IDENTIFIER, 7, "binding"), bindingType))
                    {
                        ASSERT(tokenEquals(token(TOKEN_EQUALS, 1, "="), getToken(&tokenizer)),
                               "Unexpected token!");
                        Token bindingIndex = getToken(&tokenizer);
                        info.binding = lenStringToInt(bindingIndex.text, bindingIndex.length);
                        ASSERT(tokenEquals(token(TOKEN_PAREN_CLOSE, 1, ")"), getToken(&tokenizer)),
                               "Unexpected token!, %s, %d");
                        getToken(&tokenizer);
                        Token isSampler = getToken(&tokenizer);
                        if (tokenEquals(token(TOKEN_IDENTIFIER, 9, "sampler2D"), isSampler))
                        {
                            info.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        }
                        else
                        {
                            info.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
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
printShaderBindingInfo(const ShaderBindingInfo& bindingInfo, u32* ctr)
{
    char* buf = (char*)malloc(512);
    char* cursor = buf;
    if (bindingInfo.isPushConstant)
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
        if (bindingInfo.stageFlags == VK_SHADER_STAGE_VERTEX_BIT)
        {
            cursor += sprintf(cursor, "bindingInfo.stageFlags == VK_SHADER_STAGE_VERTEX_BIT\n");
        }
        else if (bindingInfo.stageFlags == VK_SHADER_STAGE_FRAGMENT_BIT)
        {
            cursor += sprintf(cursor, "bindingInfo.stageFlags == VK_SHADER_STAGE_FRAGMENT_BIT\n");
        }
        cursor += sprintf(cursor, "bindingInfo.binding == %d\n", bindingInfo.binding);
        
    }
    if (ctr)
    {
        *ctr = cursor - buf;
    }
    return buf;
}

char* printMaterialInfo(const MaterialInfo& materialInfo, u32* len)
{
    char* buf = (char*)malloc(2048);
    char* cursor = buf;
    cursor += sprintf(cursor, "%.*s\n", materialInfo.vertShaderPathLength, materialInfo.vertShaderPath);
    cursor += sprintf(cursor, "%.*s\n\n", materialInfo.fragShaderPathLength, materialInfo.fragShaderPath);
    for (int i = 0; i < materialInfo.numBindings; i++)
    {
        cursor += sprintf(cursor, "Binding Info %d:\n%s\n", i, printShaderBindingInfo(materialInfo.bindings[i], nullptr));
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
    std::vector<ShaderBindingInfo> infos = {};
    
    parseSingleShader(argv[1], &infos);
    parseSingleShader(argv[2], &infos);
    
    MaterialInfo matInfo = {};
    matInfo.numBindings = infos.size();
    for(int i = 0; i < matInfo.numBindings; i++)
    {
        matInfo.bindings[i] = infos[i];
    }

    matInfo.vertShaderPathLength = getCompiledShaderName((char*)&matInfo.vertShaderPath, argv[4], VK_SHADER_STAGE_VERTEX_BIT);

    
    matInfo.fragShaderPathLength = getCompiledShaderName((char*)&matInfo.fragShaderPath, argv[4], VK_SHADER_STAGE_FRAGMENT_BIT);

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
    char* matLog = printMaterialInfo(matInfo, &len);
# if 1
    char vertCommandPath[256];
    sprintf(vertCommandPath,
            "cmd /C W:/VulkanSDK/1.1.85.0/Bin/glslangValidator -V %s -o %s", argv[1], matInfo.vertShaderPath);
    char fragCommandPath[256];
    sprintf(fragCommandPath,
            "cmd /C W:/VulkanSDK/1.1.85.0/Bin/glslangValidator -V %s -o %s", argv[2], matInfo.fragShaderPath);
    
    system(vertCommandPath);
    system(fragCommandPath);
# endif     
    fwrite(matLog, len, 1, f);
    fclose(log);
    printf("Press ENTER key to Continue\n");  
    getchar(); 

    return 0;
}
