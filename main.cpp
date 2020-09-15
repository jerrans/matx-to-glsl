#include <iostream>
#include <string>
#include <fstream>
#include <unordered_set>

#include <MaterialXFormat/XmlIo.h>
#include <MaterialXGenGlsl/GlslShaderGenerator.h>
#include <MaterialXGenGlsl/GlslResourceBindingContext.h>
#include <MaterialXGenShader/Shader.h>
#include <MaterialXGenShader/Util.h>
#include <MaterialXCore/MaterialNode.h>

MaterialX::DocumentPtr loadLibraries(const MaterialX::FilePathVec& libraryFolders, const MaterialX::FileSearchPath& searchPath)
{
    MaterialX::DocumentPtr doc = MaterialX::createDocument();
    for (const std::string& libraryFolder : libraryFolders)
    {
        //MaterialX::CopyOptions copyOptions;
        //copyOptions.skipConflictingElements = true;

        MaterialX::XmlReadOptions readOptions;
        //readOptions.skipConflictingElements = true;

        MaterialX::FilePath libraryPath = searchPath.find(libraryFolder);
        for (const MaterialX::FilePath& filename : libraryPath.getFilesInDirectory(MaterialX::MTLX_EXTENSION))
        {
            MaterialX::DocumentPtr libDoc = MaterialX::createDocument();
            MaterialX::readFromXmlFile(libDoc, libraryPath / filename, MaterialX::FileSearchPath(), &readOptions);
            //doc->importLibrary(libDoc, &copyOptions);
            doc->importLibrary(libDoc);
        }
    }
    return doc;
}

int main( int argc, char **argv )
{
    MaterialX::FilePathVec libraryFolders = { "stdlib", "pbrlib", "stdlib/genglsl", "pbrlib/genglsl", 
                                       "bxdf", "lights", "lights/genglsl" };

    MaterialX::FileSearchPath searchPath(argv[2]);

    MaterialX::DocumentPtr libraries;
    try
    {
        libraries = loadLibraries(libraryFolders, searchPath);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to load standard data libraries: " << e.what() << std::endl;
        return 1;
    }
    if (libraries->getChildren().empty())
    {
        std::cerr << "Could not find standard data libraries on the given search path: " << argv[2] << std::endl;
    }
    auto generator = MaterialX::GlslShaderGenerator::create();
    auto generatorContext = MaterialX::GenContext(generator);
    generatorContext.registerSourceCodeSearchPath(searchPath);

    auto doc = MaterialX::createDocument();
    MaterialX::readFromXmlFile(doc, argv[1]);

    //MaterialX::CopyOptions copyOptions;
    //copyOptions.skipConflictingElements = true;
    //doc->importLibrary(libraries, &copyOptions);
    doc->importLibrary(libraries);

    // Find renderable nodes...
    std::vector<MaterialX::TypedElementPtr> elements;
    std::unordered_set<MaterialX::ElementPtr> processedOutputs;

    MaterialX::findRenderableMaterialNodes(doc, elements, false, processedOutputs);

    for (MaterialX::TypedElementPtr element : elements)
        std::cout << "Renderable Materials: " << element->getName() << std::endl;

    if( elements.size() == 0 )
    {
        std::cerr << "No renderable materials found." << std::endl;
        return -1;
    }

    auto materialNodeName = elements[0]->getName();

    auto generateShaders = [&generator, &generatorContext](auto node)
    {
        try{
            auto shader = generator->generate(node->getName(), node, generatorContext);
            for( int i=0; i<shader->numStages(); ++i )
            {
                auto shaderStage = shader->getStage(i);
                std::string elementName(node->getName());
                std::string stageId;
                if( shaderStage.getName() == "vertex" )
                    stageId = "vert";
                else if( shaderStage.getName() == "pixel" )
                    stageId = "frag";
                std::string fn = elementName + "." + stageId + ".glsl";
                std::ofstream out(fn.c_str());
                out << shaderStage.getSourceCode();
                out.close();
                std::string spirvCompileCommand("$VULKAN_SDK/bin/glslangValidator -V -G --auto-map-locations --auto-map-bindings ");
                spirvCompileCommand += fn;
                std::cout << spirvCompileCommand << std::endl;
                //std::system(spirvCompileCommand.c_str());
                auto uniformDataFilename = fn + ".json";
                out.open(uniformDataFilename.c_str());
                out  << shaderStage.getUniformData();
                out.close();
            }
        }catch(MaterialX::ExceptionShaderGenError &e){
            std::cerr << e.what() << std::endl;
        }
    };


    MaterialX::GlslResourceBindingContextPtr bindCtx = std::make_shared<MaterialX::GlslResourceBindingContext>(0, 0);
    generatorContext.pushUserData(MaterialX::HW::USER_DATA_BINDING_CONTEXT, bindCtx);

    auto nodeGraph = doc->getNodeGraph(materialNodeName);
    if (nodeGraph)
    {
        MaterialX::TypedElementPtr outputShader = nodeGraph->getOutputs()[0];
        generateShaders(outputShader);
    }else{
        auto materialNodes = doc->getMaterialNodes();
        if (!materialNodes.empty())
        {
            for (const MaterialX::NodePtr& materialNode : materialNodes)
            {
                if (materialNode->getName() == materialNodeName)
                {
                    std::unordered_set<MaterialX::NodePtr> shaderNodes = MaterialX::getShaderNodes(materialNode);
                    if (!shaderNodes.empty())
                    {
                        MaterialX::TypedElementPtr shaderNode = *shaderNodes.begin();
                        generateShaders(shaderNode);
                    }
                }
            }
        }
    }
    generatorContext.popUserData(MaterialX::HW::USER_DATA_BINDING_CONTEXT);
/*
    for (MaterialX::ElementPtr elem : doc->traverseTree())
    {
        if( elem->isA<MaterialX::ShaderRef>() )
        {
            std::cout << "Found ShaderRef element " << elem->getName() << std::endl;
            try{
                auto shader = generator->generate(elem->getName(), elem, generatorContext);
                for( int i=0; i<shader->numStages(); ++i )
                {
                    auto shaderStage = shader->getStage(i);
                    std::string elementName(elem->getName());
                    std::string stageId;
                    if( shaderStage.getName() == "vertex" )
                        stageId = "vert";
                    else if( shaderStage.getName() == "pixel" )
                        stageId = "frag";
                    std::string fn = elementName + "." + stageId + ".glsl";
                    std::ofstream out(fn.c_str());
                    out << shaderStage.getSourceCode();
                    out.close();

                    //out.open(stageId + ".h");
                    //out << shaderStage.getInterfaceHeaderCode();
                    //out.close();
                    std::string spirvCompileCommand("$VULKAN_SDK/bin/glslangValidator -V -G --auto-map-locations --auto-map-bindings ");
                    spirvCompileCommand += fn;
                    std::system(spirvCompileCommand.c_str());
                }
            }catch(MaterialX::ExceptionShaderGenError &e){
                std::cout << e.what() << std::endl;
            }
        }
    }*/
    return 0;
}
