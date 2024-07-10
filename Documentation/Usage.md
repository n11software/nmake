# NMake | Usage

### Creating a new project

To create a new project use the following command.

> nmake new {EnvironmentName} C++

NMake will accept the following C, C++, CPP, c, c++, cpp, ASM, and asm

NMake will give you a menu to select the project type.

### Adding to an existing project

To use NMake with an existing project use the following command.

> nmake add {EnvironmentName}

NMake will automatically detect the source and build directories, and languages. If you want to change any of the options, NMake will generate a `config` file