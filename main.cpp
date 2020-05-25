#include <iostream>
#include <string>
#include <fstream>

#include <MaterialXFormat/XmlIo.h>
#include <MaterialXGenGlsl/GlslShaderGenerator.h>
#include <MaterialXGenShader/Shader.h>

MaterialX::DocumentPtr loadLibraries(const MaterialX::FilePathVec& libraryFolders, const MaterialX::FileSearchPath& searchPath)
{
    MaterialX::DocumentPtr doc = MaterialX::createDocument();
    for (const std::string& libraryFolder : libraryFolders)
    {
        MaterialX::CopyOptions copyOptions;
        copyOptions.skipConflictingElements = true;

        MaterialX::XmlReadOptions readOptions;
        readOptions.skipConflictingElements = true;

        MaterialX::FilePath libraryPath = searchPath.find(libraryFolder);
        for (const MaterialX::FilePath& filename : libraryPath.getFilesInDirectory(MaterialX::MTLX_EXTENSION))
        {
            MaterialX::DocumentPtr libDoc = MaterialX::createDocument();
            MaterialX::readFromXmlFile(libDoc, libraryPath / filename, MaterialX::FileSearchPath(), &readOptions);
            doc->importLibrary(libDoc, &copyOptions);
        }
    }
    return doc;
}

int main( int argc, char **argv )
{
    // Read a document from disk.
    auto doc = MaterialX::createDocument();
    MaterialX::readFromXmlFile(doc, argv[1]);

    MaterialX::FilePathVec libraryFolders = { "libraries/stdlib", "libraries/pbrlib", "libraries/stdlib/genglsl", "libraries/pbrlib/genglsl", 
                                       "libraries/bxdf", "libraries/lights", "libraries/lights/genglsl" };

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

    MaterialX::CopyOptions copyOptions;
    copyOptions.skipConflictingElements = true;
    doc->importLibrary(libraries, &copyOptions);

    for (MaterialX::ElementPtr elem : doc->traverseTree())
    {
        if( elem->isA<MaterialX::ShaderRef>() )
        {
            std::cout << "Found element " << elem->getName() << std::endl;
            try{
                auto shader = generator->generate(elem->getName(), elem, generatorContext);
                for( int i=0; i<shader->numStages(); ++i )
                {
                    auto shaderStage = shader->getStage(i);
                    std::string elementName(elem->getName());
                    std::ofstream out(elementName + "_" + shaderStage.getName() + ".glsl");
                    out << shaderStage.getSourceCode();
                    out.close();
                }
            }catch(MaterialX::ExceptionShaderGenError &e){
                std::cout << e.what() << std::endl;
            }
        }
    }
    return 0;
}
