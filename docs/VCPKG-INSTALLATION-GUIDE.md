# vcpkg (Package Manager)

- vcpkg [Official Site](https://vcpkg.io/)

- Overview at [https://learn.microsoft.com/en-us/vcpkg/get_started/overview](https://learn.microsoft.com/en-us/vcpkg/get_started/overview)

- Instructions on how to use vcpkg packages from within Visual Studio.
    - [Tutorial: Install and use packages with MSBuild in Visual Studio](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-msbuild?pivots=shell-cmd)
    
## Installation Steps

### 1. Setup vcpkg Package Manager ###
	
1. Clone the repository
    - Use the following command:
		- **git clone https://github.com/microsoft/vcpkg.git**
2. Run the bootstrap script.
    - Use the following command:
		- **cd vcpkg && bootstrap-vcpkg.bat**
    - NOTE: The bootstrap script performs prerequisite checks and downloads the vcpkg executable.
3. The vcpkg executable is now set up and ready to use.

### 2. Install vcpkg Packages. ###
    
- Use the following Commands:
    - **vcpkg install libflac**
    - **vcpkg install libogg**
    - **vcpkg install libvorbis**
    - **vcpkg install wavpack**

### 3. Integrate vcpkg with Visual Studio MSBuild. ###
    
- Set the user-wide instance of vcpkg so that MSBuild will be able to find it.
    - Use the following Command:
        - **vcpkg integrate install**

### 4. Set the Environment Variable and PATH. ###
    
- Environment Variable Command: 
    - **set VCPKG_ROOT="C:\vcpkg"**
- PATH Command: 
    - **set PATH=%VCPKG_ROOT%;%PATH%**

### 5. Visual Studio - Developer Command Prompt. (Optional Step) ###

- **NOTE:** *The existing Projects have already had this step completed for each of them, therefore this step can be skipped.*

1. Open the Developer Command Prompt from within each Solution/Project and run the vcpkg new command.
	- The vcpkg new command adds a *vcpkg.json* file and a *vcpkg-configuration.json* file in the Project's directory.
		- Use following the command: 
            - **vcpkg new --application**

2. Add package dependency to *vcpkg.json* (Manually add a vcpkg package dependency)
	- Use following the command: 
        - **vcpkg add port <package-name>**
			- Example: 
                - **vcpkg add port libflac**
                - **vcpkg add port libogg**
                - **vcpkg add port libvorbis**
                - **vcpkg add port wavpack**

3. Afterwards, right-click the Project that had the *vcpkg.json* file and the *vcpkg-configuration.json* file previously added to it.
	- Navigate to Configuration Properties-->vcpkg-->General-->Use Vcpkg Manifest
	- Set 'Use Vcpkg Manifest' to 'Yes'.

### 6. Build and run the Project. ###

- If MSBuild detects a *vcpkg.json* file and manifests are enabled in your project, MSBuild installs the manifest's dependencies as a pre-build step. Dependencies are installed in a *vcpkg_installed* directory in the project's build output directory.
