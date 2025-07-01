#!/bin/bash
set -e

versionMajor=0
versionMinor=0
versionPatch=2
version="${versionMajor}.${versionMinor}.${versionPatch}"

packageMD5=""
# shellcheck disable=SC2046
curDir=$(dirname $(realpath -- "$0"))
workDir=${curDir}/out
packageName="graceful-DE2-${version}.tar.gz"

[[ -d "${workDir}" ]] && rm -rf "${workDir}"
[[ ! -d "${workDir}" ]] && mkdir -p "${workDir}"
[[ ! -d ${workDir}/metadata ]] && mkdir -p "${workDir}/metadata"
[[ ! -d ${workDir}/profiled ]] && mkdir -p "${workDir}/profiles"
[[ ! -d ${workDir}/gui-apps/graceful-DE2 ]] && mkdir -p "${workDir}/gui-apps/graceful-DE2"

sed -i -E "s/^set\(PROJECT_VERSION_MAJOR\ [0-9]+\)$/set\(PROJECT_VERSION_MAJOR\ ${versionMajor}\)/" \
    "${curDir}/CMakeLists.txt"
sed -i -E "s/^set\(PROJECT_VERSION_MINOR\ [0-9]+\)$/set\(PROJECT_VERSION_MINOR\ ${versionMinor}\)/" \
    "${curDir}/CMakeLists.txt"
sed -i -E "s/^set\(PROJECT_VERSION_PATCH\ [0-9]+\)$/set\(PROJECT_VERSION_PATCH\ ${versionPatch}\)/" \
    "${curDir}/CMakeLists.txt"

tar zcf "${packageName}" ./Makefile ./CMakeLists.txt ./LICENSE ./README.md ./src/ ./data/
[[ -f "./${packageName}" ]] && mv "./${packageName}" "${workDir}"
[[ -f "${workDir}/${packageName}" ]] && packageMD5=$(sha512sum "${workDir}/${packageName}" | awk '{print $1}')
sudo cp ${workDir}/${packageName} /var/cache/distfiles/

cat << EOF > ${workDir}/metadata/layout.conf
masters = gentoo
EOF

cat << EOF > ${workDir}/profiles/eapi
8
EOF

cat << EOF > ${workDir}/profiles/repo_name
local-gentoo-graceful-DE2
EOF

cat << EOF > ${workDir}/gui-apps/graceful-DE2/graceful-DE2-${version}.ebuild
# Maintainer: dingjing <dingjing@live.cn>

EAPI=8

IUSE="test"
RESTRICT="fetch"
DESCRIPTION="graceful-DE2"
HOMEPAGE="https://github.com/graceful-linux/graceful-DE2"
SRC_URI="${packageName}"

SLOT="0"
LICENSE="MIT"
KEYWORDS="~amd64"

# 编译时需要的依赖
DEPEND=""
# 运行时需要的依赖
RDEPEND=""
# 构建工具依赖
BDEPEND="
	dev-build/cmake
	virtual/pkgconfig
"
# 安装后推荐依赖
PDEPEND=""

export BASEDIR="\`pwd\`/.."
export WORKDIR="\${BASEDIR}/work"
export BUILDDIR="\${WORKDIR}/build/"
export SRCDIR="\${WORKDIR}/graceful-DE2-\${PV}"

#CMAKE_MAKEFILE_GENERATOR="makefiles"
#inherit cmake

pkg_pretend() {
        elog "BASEDIR   :       \${BASEDIR}"
        elog "BUILDDIR  :       \${BUILDDIR}"
        elog "SRCDIR    :       \${SRCDIR}"
        elog "Pretent ..."
}

pkg_setup() {
        elog "Setup ..."
}

src_unpack() {
        elog "Unpacking ..."
        default
		elog "Move from \${WORKDIR}/graceful-DE2-\${PV} To \${WORKDIR}/graceful-DE2-\${PV}"
		mv \${WORKDIR} \${WORKDIR}.bak
		[[ ! -d "\${WORKDIR}/" ]] && mkdir -p "\${WORKDIR}/"
		mv \${WORKDIR}.bak \${WORKDIR}/../graceful-DE2-\${PV}
		mv \${WORKDIR}/../graceful-DE2-\${PV} \${WORKDIR}/graceful-DE2-\${PV}
}

src_prepare() {
        elog "Prepare ..."
        eapply_user
}

src_configure() {
        elog "Configure ..."
        cd "\${WORKDIR}/" 
        mkdir -p \${BUILDDIR}
        cmake -DCMAKE_INSTALL_PREFIX=/usr/ -B "\${BUILDDIR}" "\${SRCDIR}" 
}

src_compile() {
        elog "Start compile ..."
        make -C "\${BUILDDIR}" -j\$(nproc)
}

src_test() {
        elog "Start test ..."
		#make -C "\${BUILDDIR}" test || die "test error!"
}

src_install() {
        elog "Start Install ..."
		cd "\${BUILDDIR}/"
		DESTDIR="\${D}" make -C \${BUILDDIR} install
}

pkg_preinst() {
        elog "Preinstall ..."
}

pkg_postinst() {
        elog "Postinstall ..."
}

EOF

cd "${workDir}"
ebuild gui-apps/graceful-DE2/graceful-DE2-${version}.ebuild digest
sudo FEATURES="test" ebuild gui-apps/graceful-DE2/graceful-DE2-${version}.ebuild merge
cd "${curDir}"

