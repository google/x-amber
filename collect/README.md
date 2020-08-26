# NOTICE
* The materials covered by the associated license relate to a general purpose product and its underlying designs, schematics, and hardware and software code (collectively “Contributions”), none of which have been evaluated, cleared or approved for any medical or other purpose by and may not meet applicable safety or other legal or regulatory requirements of any governmental agency, including but not limited to the United States Food and Drug Administration (“FDA”), or equivalent regulatory bodies outside of the US.
* Any use of the term “EEG” or “electroencephalogram” contained in the Contribution is for informational purposes only and does not label, represent or otherwise indicate the Contribution is intended to be used or has been reviewed or approved as a medical device by the US FDA or equivalent regulatory bodies outside of the US.
* Any article, material or other publication by any person or entity referencing the use or operation of the Contribution for a medical or other any other purpose is for informational purposes only; does not directly or indirectly represent or infer any intended use in the diagnosis of disease or other conditions, or in the cure, mitigation, treatment, or prevention of disease, in people or animals; and does not label, represent or otherwise indicate the Contribution is intended to be used or has been reviewed or approved as a medical device by the US FDA or equivalent regulatory bodies outside of the US.
* While the Contributions may have potential medical applications, many countries, including the US, require regulatory approvals prior to using any device in the diagnosis of disease or other conditions, or in the cure, mitigation, treatment, or prevention of disease, in people or animals.
* It is the user’s responsibility to secure all necessary regulatory approvals and meeting all applicable government safety and environmental standards associated with use of the Contributions.

# Collect
Collect is Amber's EEG recorder and visualizer.

## Getting started

### Install dependencies

#### Linux/Mac
##### 1. Install Node Version Manager (nvm)
```
curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.34.0/install.sh | bash
```

Ensure that nvm installed correctly by running `nvm --version`, which should display the version of nvm installed.

##### 2. Install Node.js Version 11
```
nvm install 11
```
Note, Node.js 12 is not yet compatible with Collect.

##### 3. Install bower
```
npm install -g bower
```

##### 4. Install yarn
```
npm install -g yarn
```

#### Windows
##### 1. Install Node Version Manager (nvm)
Follow the instructions [here](https://docs.microsoft.com/en-us/windows/nodejs/setup-on-windows).

Ensure that nvm installed correctly by running `nvm --version` in PowerShell, which should display the version of nvm installed.

##### 2. Install git
Follow the instructions [here](https://git-scm.com/download/win).

##### 3. Install Node.js Version 11
```
nvm install 11
```
Note, Node.js 12 is not yet compatible with Collect.

##### 4. Install Windows build tool
```
npm --add-python-to-path='true' --debug install --global windows-build-tools
```

##### 5. Install bower
```
npm install -g bower
```

##### 6. Install yarn
```
npm install -g yarn
```

### Clone repository
```
cd collect
git clone https://amber-internal.googlesource.com/collect
```

### Run collect
Install modules:
```
bower install
yarn install
./node_modules/.bin/electron-rebuild
```
Run collect:
```
yarn run start
```

### Build collect binaries
```
./build-me
```
