const u32 RANDOM[] = {
0xd6d807e1, 0x85bb13c5, 0x03541c3a, 0x7c613aed,
0x4e57084c, 0xdd05cd88, 0x1d962a98, 0x0968a328,
0xfc08550f, 0x89c5f943, 0x6242ab1d, 0x61c1fc3e,
0x975fafb0, 0x3a06a539, 0x490a41bb, 0xdcff38d9,
0x78fd3e2b, 0x6177cb39, 0x18d07734, 0x891bed77,
0x6b81065f, 0xcb689854, 0x78c30560, 0x8322fa0c,
0xcc745b0e, 0x9b725575, 0xec45cf36, 0xddd553ae,
0xbc4509e2, 0x2f579538, 0x98422ea1, 0x9b62d2f3,
0x6e8e75b5, 0xf5f39569, 0xdaaf2c20, 0x59826eb8,
0x64bcbe31, 0x3830326b, 0xddede35d, 0xdb0351fe,
0x155b562e, 0x99fbb54d, 0x4d6c3e2a, 0xebd94bb3,
0xc90b3cb4, 0xf86d179d, 0x3fdcbfd0, 0x2f80af72,
0x1addbf16, 0x289a9e4a, 0x50b282cc, 0x1c361caf,
0xe34f90e0, 0xe7143d48, 0x2712a8e4, 0xdd5a5d6a,
0xf4b6b5fe, 0x7b641b8a, 0x1472e383, 0x936d31e4,
0x08e8aa61, 0xfb5d15d7, 0x925742a7, 0x4e8fb87d,
0xc0eca965, 0x23c4a993, 0x3a49917a, 0x345fddbb,
0xe38c3c84, 0x2dd27542, 0x399dc338, 0xf34568c8,
0xbcdcf9d1, 0x04c8d655, 0x00f3bd3a, 0xfc0e48ba,
0x0a6c7b09, 0x7d078970, 0x274c5726, 0x370a1f83,
0x122bf31d, 0x3d6edd32, 0xa0839b45, 0x88c74bbb,
0xa2c6de3f, 0xdf58ad15, 0x8109c0b5, 0x2d119a27,
0x1f4997ae, 0x709df48e, 0x15ae304f, 0xbae6d988,
0xca128610, 0x6a8711f8, 0xd870188e, 0xe0ab029e,
0x0a418d35, 0xc4cbcd74, 0x6c663dda, 0x53d4a04f,
0xb51113f8, 0x3e81eaa7, 0x7f9300cb, 0x5d8d44ae,
0x1498cc82, 0x94b469ea, 0x8937a16a, 0x1e77819b,
0x3e041452, 0x96b8be88, 0x9edbe724, 0xf4b9b09f,
0xfd07b65b, 0x8527c9a6, 0xcc0fe89f, 0x8791d2ac,
0xa8a14c74, 0xbac07d1d, 0x86783e1d, 0xa1a8696f,
0x08d15d8e, 0xf23a869b, 0x98aa5d4f, 0xa8b61203,
0x5c98906a, 0x2aa4df65, 0xbed5020e, 0xc679f112,
0xd87cf894, 0x30b06a9d, 0x4bb9b470, 0xd946320b,
0x1ba36335, 0xf98ddb4c, 0xd28c0fab, 0xc5b74192,
0x5852b686, 0xce5dc145, 0x1e37611b, 0x2c3f692c,
0xe2f18f7b, 0xd3b62c12, 0xbc6425b5, 0xf28a94ed,
0x2caaa269, 0xf0020b67, 0x8dce0f6b, 0xa7181d0b,
0xa259214d, 0x60448089, 0xffe3bd93, 0x06a23abc,
0x7a2ecb58, 0x4fdcee24, 0xa6abd949, 0xf4f5a67c,
0x90e3b04c, 0xbbac459b, 0x62d0360f, 0x8569a0ea,
0xc25a859a, 0x7b2603b9, 0x592f407d, 0x0a296d8e,
0x244873a3, 0x1f86351e, 0x03386556, 0xa1075a10,
0x7eff5504, 0x3c830917, 0x320978c5, 0x151cadbc,
0xc3ad9cd8, 0xa7982a5f, 0x2aa804f4, 0x5fa92c0d,
0xc1ed2863, 0xf414fe60, 0x01d69621, 0x56f23652,
0x5ca40be7, 0xb2d76cce, 0x779eb886, 0x04bf5e3a,
0x4a794974, 0x9f1df673, 0x7f761691, 0x6ba800c2,
0x2089a28c, 0x3a62a024, 0x48a727b9, 0xd774259b,
0x2a596b9a, 0xb291e770, 0x7dee2a4a, 0x8be92b3b,
0x2d3a7539, 0xdf499f9f, 0x3bd8a1cb, 0xd3707990,
0xe5576f4c, 0xf37bd282, 0x74cba0d6, 0x3cc9eab8,
0x2e3dc996, 0x69461fe9, 0x34c57574, 0xb4a966c9,
0xc5cbc03f, 0x8cf137f7, 0xd1f63f6c, 0xb683b9a7,
0xf308ed4e, 0xe0af8afc, 0xf0152340, 0x18380bfc,
0x9e96bd4f, 0x19233c4d, 0x2d4940a8, 0x6b2881c2,
0x2c134acc, 0xaa457d61, 0x25848122, 0xc10e3740,
0x32f657fd, 0xae638a5b, 0x56ca0b10, 0xd00f9e54,
0xf035029e, 0x81683eba, 0x84c6c509, 0xb52dbcef,
0xca83fdb6, 0xcbf2ce2e, 0xb3f32265, 0x6afe9dbf,
0x519758f5, 0xd4442831, 0x8cb4d492, 0x676fddb7,
0xdd2dedee, 0x9ced5ff7, 0x2a37b7a9, 0xc4adb085,
0x4997123f, 0xf947f6b9, 0x5187aa3e, 0x46d72a1a,
0x7aeb7670, 0x208f666d, 0x615826f1, 0x1189d3cd,
0x64021532, 0x4b5b0577, 0x0ce236cb, 0xc2db6d1a,
0x73fadc5c, 0x652e345a, 0xbd172385, 0xee8f4d68,
0x6956e0eb, 0x1e4469b1, 0x71a2c02a, 0x4c01b91a,
0xf2f56919, 0xda52eada, 0xcc39253e, 0x363ac2dd,
0x38f46e1d, 0xa17e7b0c, 0x0725b1bb, 0x7ace6dbc,
0xa17ad191, 0xb619833f, 0x18cdcbbf, 0x0da21e16,
0xb8042efe, 0x69bb1306, 0x9cb25a15, 0x65475172,
0x5dcfb1cb, 0x6a4a382a, 0x16edca26, 0xc69be309,
0x98b05db8, 0x87db6419, 0xad44cc13, 0x0cb009c1,
0x760e0a21, 0x1ca0aa8a, 0x336582e8, 0xd792ba50,
0xebfa8fb8, 0x090b6428, 0x8dbf9188, 0x0f5355a7,
0xd24e734e, 0x15a27848, 0xd0df61c7, 0x24d30217,
0xb65f976b, 0x62deb833, 0x4318c020, 0xce2dd30c,
0x0eeb238e, 0xb4e788ce, 0x89b9cf2c, 0x9aaad6c3,
0x1d52e753, 0x25bfea29, 0x7942e481, 0xeb9b949f,
0xfca9025f, 0x2e5cc365, 0x6da7d5db, 0x660f7062,
0x87aaedc0, 0x6f912298, 0xbd1b4d76, 0xccc640cd,
0xb977ad50, 0x93640118, 0x2ab8f635, 0x97ccc164,
0x1fd896fa, 0xa135d83e, 0x88b02c36, 0x84cc59b6,
0x9b050a3f, 0xe6a29ac2, 0xcdae561d, 0x5f298ff0,
0xdc2adb4f, 0x5bce5483, 0x1fc1d3d8, 0xd65d2df0,
0xc116de00, 0x7b4c124d, 0x9cc61948, 0x3697d07b,
0xfa689d47, 0x2cabe892, 0x48e5059a, 0x6223b67a,
0x657464ee, 0x2bd8603d, 0x3d6f6eda, 0xeed76e11,
0xb21bb102, 0xd05c03da, 0x2891e959, 0xe7db2172,
0xdcb583c2, 0xb795c1e0, 0x9faf7312, 0x074eb67c,
0x01ab24b0, 0x86479456, 0x4ab3c3f7, 0xe03efc10,
0x5d202799, 0x9c418d50, 0x29d1b76c, 0xf3cddbfb,
0xd541a225, 0xa92debe3, 0xbd3622b6, 0xbc542042,
0x6e3ec28d, 0xadb8dbb7, 0x2efe7040, 0x4281db49,
0x37e78a98, 0xcfe88fdf, 0x5a79fe60, 0xe1363b3b,
0x98900c40, 0x4521c85e, 0xddc5d72c, 0xc968d560,
0x5ec56b0a, 0x5307614d, 0x03b5fb58, 0x6479238f,
0x5309e408, 0x1e6f63de, 0x08987310, 0xad303f0a,
0xd6974e09, 0xa7ff4ed0, 0x503a8282, 0xfb5f008e,
0xbd9dbb64, 0x9478d385, 0x7937bd3d, 0xddae7e54,
0xe1742306, 0x9596fe72, 0xe8923a1c, 0x0b1c015d,
0xb26968f4, 0x92766fd9, 0x2959530d, 0x4a378fee,
0x82e082b6, 0x818d89ae, 0xd6a7087a, 0x830c1680,
0xb804aafe, 0xf9ae11ad, 0x22172e56, 0x47a3c555,
0x2fa95761, 0x50867086, 0xa7292fd0, 0xf88ce4ee,
0xf7961efa, 0x95a5e49b, 0x567e4518, 0x37dcedd4,
0x18c60aea, 0x6747fa4d, 0x8c48fc3a, 0xc703eebf,
0x510ef46e, 0x659cac9b, 0x830ca8c0, 0xa8240b9e,
0xc2e9d8d2, 0x8e10e0bc, 0x2c68b11f, 0x7e78dbcf,
0xc112467e, 0xcf73d17e, 0x72bcfb0c, 0xda9292d4,
0xccaa53ac, 0x4706f9dd, 0xde13ede8, 0xed7ec423,
0x97f5b037, 0xe0f0f1fc, 0x86db6d68, 0xd07720d6,
0x6a0beae3, 0xc22c4695, 0xc90ada09, 0x0feb9112,
0xe3215014, 0x4a7fb80f, 0x098c0a17, 0x8c8aa592,
0x1e97ce6b, 0xd7713df4, 0xfb03b0a7, 0x17ebc3cf,
0x9c473e08, 0xe32722b7, 0x38028fcb, 0xc051e1af,
0xa2595675, 0x985fe159, 0x74c7b3da, 0xc76a4932,
0x71a8d4e0, 0x73561832, 0x30350afe, 0x7a8d2abd,
0x437aa384, 0xc4503e5a, 0xb648f028, 0x937ba5ce,
0x4f48ed03, 0x839680cd, 0xb815bec9, 0xa14f915d,
0xda175429, 0x3b0e885c, 0x0d416466, 0x4e6eca14,
0x2ccc5104, 0xa65392db, 0x58e8d34a, 0xaa5986a8,
0x62341fd2, 0xa4208ed9, 0xd995d4d0, 0xae8878d5,
0xe3bf2f0d, 0x6dc7c45e, 0x38b63217, 0x2408cf2e,
0x2fdcc2f2, 0x8bbe1569, 0x468b5489, 0x2c1590c9,
0x9ca064d6, 0xf4c9568d, 0xdba52aa9, 0x3f81483b,
0x5733ece6, 0x5d1d2985, 0xef8b3810, 0xe622740d,
0x694554c5, 0xd9bc46e0, 0x7febd0f5, 0xa23136bc,
0x4c750837, 0x0226bef7, 0xf312b90b, 0x937a8bfa,
0x40b22785, 0xb5c5e407, 0x0299a340, 0x387aca84,
0x9209bcfa, 0xfe03061b, 0xa6f6a9ae, 0x009c236d,
0x1c163d00, 0x0fd1a21b, 0xbf456b30, 0x184df181,
0xfdfa09db, 0x2cf9e0ca, 0x76cfb7fe, 0x6db041ae,
0x05f728aa, 0xcc439e1e, 0x43e96115, 0x8a762b1b,
0x9606c532, 0xaa40c8d1, 0xfc3b8af5, 0xb2cedc0f,
0xe136617e, 0xa6fbadf7, 0xbd70f169, 0x3b130dd6,
0x407436cb, 0x30644edb, 0xa1e44b7c, 0xbc32c4a4,
0xf6f69fc6, 0xd5aa588e, 0x0007308a, 0xac91d12b,
0x1352ffe9, 0x17769145, 0x9fa6368f, 0xd1055f1c,
0x177c0442, 0xb8945492, 0xa189f7df, 0x83a1e846,
0x37adad22, 0x11b5fc9e, 0x777c3a98, 0x48c76494,
0x490de51d, 0x8866216c, 0x789ae589, 0x0f5e69ea,
0xcecc8f8a, 0x631cc22e, 0xedf97f42, 0xba4e1039,
0xb12398c8, 0x41a4f8c2, 0x05704329, 0x05d3f6ca,
0x00fc4d7b, 0xefe58fae, 0x958a8b3c, 0x80733b0e,
0x91ca8ed6, 0x14df7632, 0x9d40dca2, 0x0ee4930b,
0xbe81e7d8, 0x8a5baa06, 0x9e13cc32, 0x36f421b6,
0xba8eb974, 0xf3e6dfb2, 0x133cf13a, 0x5d2c7d82,
0xd8324e26, 0xaab7d33b, 0xea53996f, 0x46082e5e,
0xd95bfc16, 0x6fc0480a, 0xc9b5351e, 0x85d7b023,
0x7c32c5fb, 0x5810c16d, 0xb4b5e0ef, 0x634a5425,
0x107abefd, 0xb400e819, 0x7241bb5d, 0x2e73734f,
0xccfbf462, 0x019038ac, 0x5645dff0, 0x4163dab1,
0x11572e05, 0x23fe217f, 0x45b7ce86, 0x306f5a65,
0x3d924da1, 0xddf8244c, 0x53adf3db, 0xdeac5f74,
0xaac67399, 0xdc94f9a1, 0x8372f2fd, 0x55f4ba70,
0xc35f7b4d, 0x029e9e87, 0x5279eec7, 0x24283c40,
0xd2beed05, 0xdae43685, 0xf3b767dd, 0x88bf580a,
0x1184da29, 0xc1c6ee13, 0xd31cd233, 0x06113138,
0xbbd3dad8, 0xd3d542a2, 0x680c4f76, 0x3d874de9,
0xa6b4fb3e, 0x717b1627, 0xfb58486b, 0x146af79d,
0xb695c6c7, 0x26c12bd6, 0xd0f2b4b6, 0xe03accb5,
0xb5ac07ba, 0xa5a86e47, 0x2dac8724, 0x6ffb25e6,
0x87adfabe, 0x80fc4f95, 0xd1fc1d33, 0x334f1160,
0xadcb4091, 0x921c301f, 0x9936f2fc, 0xc565c565,
0xb3d81c58, 0xdb44ba25, 0xa06f315a, 0x77e6e470,
0xe34029c6, 0xe92d4a6a, 0x0f7fea44, 0xcbfde64d,
0xa58d5e6a, 0x31fdf939, 0xf646f62c, 0x13683c45,
0x62e4909e, 0x1988f047, 0xddf77264, 0x2771503c,
0x3ebd4a3e, 0xd57b06fa, 0xad5f8cc2, 0x1a24b592,
0x1988fd13, 0xe70c846a, 0x8976c22e, 0xe393deef,
0x3bd5f609, 0xf3073135, 0x2c5e714e, 0x19179b84,
0x9a78ff38, 0xf73bab3e, 0x2115bfc2, 0xc296169a,
0xd8d41d1d, 0x9b73cf3b, 0x9c32cc04, 0xcf5e7404,
0x7d919502, 0x3b256469, 0xb4e12f96, 0xf5053c9b,
0x4d3ae8ba, 0x0c6aae98, 0xe8e36307, 0x41251298,
0x200d4909, 0x2abc1b81, 0x7e547bcd, 0x2fcfaf8c,
0x044ce2d1, 0x23183e7e, 0x0cdcab65, 0xcc8b9ffb,
0x10f7f6c3, 0x55985166, 0x81af5925, 0xfbe5e1bb,
0x1f5c9212, 0xdb05b37c, 0x337616bc, 0x8ca501ea,
0xbcb73850, 0x3fa0b67a, 0xce0a313d, 0xe8127930,
0x6b0c4963, 0x4b9512f0, 0x6e5c6277, 0xf631ddc7,
0x88bd6cf6, 0xbd0eca43, 0xab5bbbf6, 0xab841760,
0x17f97fe4, 0xf6634d1c, 0xf9a61e0b, 0xa7503328,
0xbe636241, 0x8ce4f8ce, 0x2bae45bf, 0x4bda687f,
0x5691435a, 0xe172715b, 0xdf0262c8, 0xe1d8058c,
0x61c5b024, 0xe8ea2153, 0x956935ac, 0x72fc0f70,
0x813a8e48, 0x05dd886c, 0x9d01c130, 0x929c7082,
0x7a6cac8d, 0x2857f192, 0xbae7475d, 0x18ea69f0,
0x10b25239, 0x910fafb3, 0x81123d7f, 0xe2c3c65f,
0x46eeaa5e, 0x5941e796, 0xeb0ed2ab, 0x0c115755,
0xaafd75f5, 0x5f35748f, 0xc7d331a8, 0x9ecf91a7,
0xb14ba0c3, 0xf03e997f, 0x06553e6f, 0x53f74ef6,
0x497f289f, 0x997d856a, 0x0cc5765d, 0x75699b5b,
0x87e07fb3, 0xd2fd7868, 0x68a4e8ea, 0x648fe34a,
0xab6e2bcd, 0xcd6723f3, 0x86ebd9b8, 0xd1bd64e2,
0xf1f629fd, 0x02fb978e, 0xf9fa44ce, 0x32265481,
0x668932f9, 0xf7b3aff1, 0x4ec91e98, 0x5437932c,
0x14aafb5e, 0xa5b709a2, 0x9d935da7, 0x8898f52e,
0xb411647f, 0x32994d02, 0xdaf3a0df, 0xe41f015c,
0xef61f87e, 0x8fd0957d, 0xdf3d6fd5, 0x3ebcb0a8,
0xe0edea89, 0xf57eed31, 0xa391290f, 0x07af2790,
0xf6b98c47, 0xbc37a31a, 0x8a7e35a3, 0xb407fff9,
0x21e0a346, 0x1853437c, 0x6e1dd796, 0xdc89e5dd,
0xe8da0e99, 0x99dbad4e, 0x1f46d939, 0x96726deb,
0x0b2e130e, 0xccc499cd, 0xb9d7d547, 0x2aeb2716,
0xfe6bfb07, 0x4bc8c8b6, 0x40bd1623, 0xda41fb23,
0xf709f094, 0xfc67e2aa, 0xbab5fa6c, 0x78a5fe51,
0x93b53785, 0x92c3e800, 0x606833d8, 0x1cd7da60,
0xd14b9f67, 0x6fb0568e, 0x9cdd522c, 0x18744d66,
0xa0a62579, 0x0506ff05, 0xddf5d239, 0x107726a7,
0x63fb5c93, 0x76ffc13d, 0x8440a268, 0x851cf1ec,
0x0b895a56, 0xef771f11, 0x8e4c6ace, 0xec8ed337,
0x01b060d7, 0xfe20d33e, 0x984565f2, 0x6a7796ec,
0x3f35a2ca, 0xf0cac2de, 0x7c52ae98, 0x952d291f,
0xe24dad44, 0xe8d189d5, 0x6ad04426, 0x84d481da,
0x9a71e3fe, 0xb0207f7a, 0x61071e8d, 0xf6f65cfe,
0x9923af80, 0xd6babf14, 0xa8fe7092, 0xb3fbcb74,
0x8496aa5b, 0x0b3cedd4, 0x1747dc53, 0xf555a90b,
0xf8ffe2d6, 0x41767e7a, 0x4ae9e7cd, 0x37c04a29,
0xa366f55c, 0x1d11f464, 0x1826221b, 0x25d9bf82,
0x65664ec7, 0xd04cfcea, 0xf119b72e, 0x4acc6a45,
0x8cef22aa, 0x4bb510aa, 0xc7a95637, 0x561c673a,
0x196e1873, 0x4d155d67, 0x099b10fc, 0xf823c455,
0x7348cf2e, 0x2729466a, 0x077eb041, 0x6877613b,
0xd416e539, 0xc65d67cf, 0x25465a87, 0x9714d48a,
0x4b2de8e4, 0x24a06f8f, 0x2755102b, 0xa6bf3c71,
0x09e867f3, 0x5dcee94a, 0x1e4de77f, 0xac6ee69b,
0xe54c6484, 0x1b9ce3a6, 0xbb45545f, 0x902127b3,
0xe10ce91f, 0xf71201f8, 0xdbcc5c15, 0x52ef7220,
0x09b9ca85, 0xc0250bfa, 0xbc0370cd, 0x377687cc,
0x82a3e319, 0xb86d9113, 0x1ac80a96, 0xc8488537,
0x38d46383, 0xef19f7a6, 0xc41a2a0f, 0x96b2a664,
0x54cc377b, 0x0806e3ee, 0x164f959e, 0x8bdd5958,
0xa689f9a0, 0x201ca6cb, 0x94a6d174, 0x873b6589,
0xa18f9d20, 0xb3094eda, 0x522e8e46, 0x966d39ab,
0xddddc9a0, 0xd0cf8476, 0x87978c72, 0xb68246c2,
0x7de81508, 0xecf41488, 0xf86f8101, 0x1b0ec9d8,
0x52b0709a, 0x96c134a1, 0x8eb61d82, 0x35986e03,
0xa20eff1e, 0xaf8580bd, 0x5423b183, 0xeb6e7895,
0xcc8f0546, 0xc5972d52, 0x36675632, 0x655b99a9,
0x7832560c, 0xf06fcbad, 0x2c6708df, 0xd6a5f6d4,
0xbd376cb9, 0x5b52366d, 0xca5b870b, 0x39bf10b6,
0x72047c1c, 0x735c62fe, 0x7cbed00e, 0x64e7ca64,
0x7eb415d8, 0x83491309, 0x7707939c, 0x78bd7554,
0xbd060591, 0xf429703e, 0x88dbdab3, 0x26239603,
0x8c7a3dbe, 0x4f301f22, 0x8e1b2da5, 0xdb828eae,
0x90680765, 0x75d99f69, 0xb0739745, 0x3943d122,
0x34cbe258, 0x1551921c, 0x5adb8d06, 0xd01186b0,
0x401dafb4, 0x2c844540, 0x750814a1, 0xa005a6cd,
0xef14ba50, 0xf4b8e911, 0x0f1b4672, 0x1cb77f93,
0xe440adda, 0x2e1e8a18, 0x0dffcef0, 0x59bec55b,
0x7a3537e2, 0x954ff794, 0x7768eac5, 0xa7a3de7f,
0xbdf074ad, 0xd075c230, 0x8272f966, 0x10bf3394,
0x5333545b, 0xced2c983, 0x79d46e7f, 0xec207326,
0x959f1301, 0x6f909c39, 0x5fe3700f, 0xff01a15e,
0x7fc1f43d, 0x6ecc3f61, 0xb6f686d3, 0x3b8c3830,
0x74b7a474, 0xcbfb6fb5, 0x98cf800c, 0x4aef056d,
0xdf43bbcc, 0x708e0110, 0xd5962b24, 0x2ee8ceba,
0x74a79736, 0xeb001387, 0xdf2e1e48, 0x49180908,
0x2bfc09d9, 0x7da49806, 0x93e00ee0, 0x8648d137,
0xe1772a1f, 0x280681a6, 0xa7ac16cf, 0xb7e658d8,
0xe2153c36, 0x1780a149, 0x4c38d1ac, 0x38acf95e,
0x1c30ceea, 0xaeb992c5, 0x7e3850da, 0x9cc583ca,
0x89255fc5, 0x961bbd7c, 0x6ff80350, 0x3c07f96e,
0x5b4aea61, 0x3c561cc4, 0xd494526c, 0x601ed113,
0x9b109fa2, 0x3ac3da68, 0x96b5ecb5, 0x2b3de914,
0xa1f219a2, 0xdd367a0a, 0xa2c130a7, 0x86d02269,
0x53980882, 0x2eb73f1e, 0xdc66afa2, 0xa04f106b,
0xff913af6, 0xcd2c5790, 0x0ebc3891, 0xdfaf5353,
0xbafaa579, 0x2a060af7, 0x39b2acc8, 0x6a241230,
0xba2183f5, 0x49a20529, 0xe720efbf, 0xe834366e,
0xc09e9211, 0xc203bc6b, 0xe9d662ea, 0x310bcf9d,
0x51329bdf, 0xe1fa2157, 0x8a58da47, 0xfeb83fd1,
0x1f2e16e5, 0x573386ff, 0x06e11776, 0x0ce7c290,
0xb808face, 0x5084057b, 0xe43a4626, 0xc7e63f0d,
0x69add620, 0xa98b4b87, 0x006d4123, 0x020620cc,
0x3da62359, 0x3a1a62f6, 0xd3b0e8bc, 0xe78ee09a,
0x20e2dcf0, 0xa5633c77, 0x508e76e9, 0x2f4c03ba,
0xffa6876c, 0x2741c53f, 0x9737195c, 0xb54c8c0c,
0x75310779, 0xd381aace, 0x7fad90c6, 0x662769cd,
0x9f37da7f, 0x2f9bd480, 0xd6848687, 0x003b6faf,
0x6c63aeb2, 0xc355b64a, 0x3238d412, 0x135ec99b,
0xad4e7ed0, 0x286e339e, 0x72df294f, 0xc1920319,
0x2ac08d9d, 0x2ba660b7, 0x5fa0bd30, 0x8ebc214a,
0x3649612c, 0xb754da6c, 0x05c4d134, 0xa6e670a5,
0xc3867856, 0xf3f33702, 0xdc243904, 0xa98859a3,
0x07c358de, 0x00473c7c, 0x6d4e060f, 0xd5cdba61,
0x9c613bb0, 0xa39d309a, 0xf02c1e61, 0x6bdb9aca,
0x3082ff43, 0x146096f3, 0x09393b87, 0x90467c07,
0x2f8c036b, 0x6bd9d3d4, 0x1da659b1, 0xa66d5cb7,
0xdea23b6c, 0x0f9396be, 0xf456ea06, 0x9f5f5654,
0xe974a8d6, 0xcc4f738d, 0x0896c658, 0xe6d73dda,
0x8cc79a66, 0x5a9d0e26, 0x9b7d6ee2, 0x45391df3,
0x553badfc, 0x94f74840, 0x3fd777f2, 0x48844bee,
0x49f54283, 0xf9a6d1e8, 0xf148a4c0, 0x24b05f4a,
0x3e70544b, 0x96c5df04, 0xddde76ae, 0x5622f5d0,
0x360550f3, 0xb024fc3c, 0xf28db505, 0x388c58d7,
0x4b663792, 0x654e5708, 0x3d4f4632, 0x0110d8ef,
0x2210fa5f, 0xf2f13874, 0x76169e07, 0x0e6c7be4,
0xd0017bc0, 0x6f9c5d38, 0xbbe321c1, 0x29c2b4c6,
0x510871ca, 0xcbfb07bf, 0x780348fe, 0x416584db,
0x10594296, 0x9d8985c4, 0x18a86a10, 0x3c18479e,
0x202579bf, 0xda3f850b, 0x509a236c, 0x1272cb19,
0xd5bfc2b6, 0xd7d2f66d, 0x4de630e4, 0x7bdb5ea4,
0x580583cf, 0x386d9f00, 0xca70142d, 0xbde387ea,
0x82fe4aec, 0x3bb1aee7, 0x0951e911, 0x2e644ddc,
0x7c6f847f, 0xb4eb7d00, 0x3d12d6cb, 0xfb24bf6f,
0xf85f0cb5, 0xe00d5c8c, 0xaaef065c, 0x00bcfeda,
0x65bc830a, 0xcfa34a31, 0x803058a9, 0xa7f6c0de,
0xdc4688ca, 0xaf920fe6, 0xb18f9114, 0xfb396ef5,
0x63efbe7d, 0x91434d11, 0xd29059d4, 0x160fcaea,
0x2e10a259, 0x5f5af4d2, 0x7c72ff78, 0x3a19f821,
0xb4e4f7c7, 0xd3f8b278, 0x8012a5db, 0x22488690,
0x1bc8d6e7, 0x23d07654, 0xa1608b1c, 0x36c5734f,
0x90009a33, 0xcacf65ec, 0x1272b63e, 0x833772e0,
0x995e5aa3, 0x35c61137, 0xb19f33e2, 0x85ebf07b,
0xaae64135, 0xc813f52c, 0x3329492f, 0x0dddc847,
0x461588a9, 0x44db2d3f, 0x5d57c990, 0xff74edc2,
0xacaa56b0, 0xeedad7b2, 0x0c26d8b8, 0xf8b82158,
0x525fa826, 0x25a241aa, 0xea48806f, 0x9a25f5da,
0xcafc401e, 0x631314a0, 0x3329108f, 0x6386e05e,
0x8a525690, 0xd2e71b2a, 0x7fa0f794, 0xc31aa84e,
0xa52d0d45, 0x44636a5b, 0x0581f981, 0xdc27da34,
0x362d63a0, 0x5313a809, 0xdfc2b5ec, 0x026c1490,
0xafcb121d, 0x60121097, 0x9f96e5c8, 0x66840ebe,
0x360e084c, 0x837780dc, 0x4fb8b2cf, 0x7e15e1ed,
0xb1649224, 0x6bd30013, 0xe76ad2b3, 0xfe7dcf71,
0x9734e97a, 0x785905f8, 0xd2f87238, 0x5f7d46b5,
0xb9b17d1e, 0xba08773f, 0x7e52c32e, 0x5a87ddbb,
0xce0f36cb, 0xae8ff8cd, 0xc6a8c44f, 0x76faaf1a,
0xc2a84fa5, 0x1bab7f07, 0x0ea6c706, 0x55176c15,
0xb776fc3a, 0x16ac3e67, 0x470754c9, 0x7951e8a9,
0x1a5765ca, 0xaabb7dab, 0x6ec8cb81, 0x140c54aa,
0x5cb4b0bb, 0xce7b662b, 0xa437290a, 0xad3ae29e,
0x66e7cf36, 0x364596d3, 0x5c0005b4, 0x2104366f,
0x694c1ce7, 0x50c752fb, 0x11b297c1, 0x22a38c53,
0x68c3aa6a, 0xb73383d6, 0xfbad88e9, 0xb8af8228,
0xa81ccd80, 0x5cc199c3, 0x95ab013f, 0x744f4299,
0xe7799b33, 0x5aee95e8, 0xa413555f, 0x45136c46,
0x0961219c, 0x2065fb87, 0x263d51ee, 0xa91af0d6,
0xc27055d8, 0x63bf8878, 0x7c475faa, 0xa1e9da84,
0xb9644c58, 0xb81bc0d8, 0xd848ee76, 0x994b4913,
0x6abaf56f, 0xd6987fe4, 0x6f99b9e2, 0xc697853b,
0x2b512fd9, 0x48fc09a7, 0x73b525ce, 0xd2c060fe,
0x910e9b2f, 0x9f5c00db, 0x8329c9b9, 0x437149e7,
0x7b6e401b, 0x8b6ccade, 0x761235b5, 0xa287dbc5,
0x8b88436c, 0x826dd758, 0xebed6c0b, 0xf8361678,
0xeae2c7a4, 0xa35bfd23, 0xcfd2874f, 0x6130fc31,
0x3a2e2f9e, 0xc0790146, 0x655edc34, 0x38003085,
0xc5004ee7, 0xdac087e8, 0x2899bfe5, 0x41c5375b,
0x9e74869e, 0x810f8da8, 0x563cecf2, 0xe8036f66,
0x3b34ef93, 0x3dadda4e, 0x2e7d12a6, 0x96453026,
0x2b287101, 0xd8a601af, 0xab36571e, 0xb78fa983,
0x42d3097b, 0x896ccce6, 0xdca753ba, 0x4dcff284,
0x4a55185a, 0x7b4ceb86, 0x863ee7ed, 0x96b0e3eb,
0x83a24a6d, 0x950b8755, 0x7a6f96a4, 0xb334a2a4,
0x59c94934, 0xb59a8c65, 0xbf3027e3, 0x9b3984ce,
0x1c046ed3, 0x6697599f, 0x9ec618df, 0x056ea0b3,
0x0b1415bb, 0x05179ab7, 0xbf58470a, 0x2b6e53ea,
0x37470324, 0x5bc6fae9, 0x918d290f, 0xe7ae64ac,
0xbbc0f621, 0xf7681865, 0xcf28db8b, 0xcb4754d2,
0xa86420a6, 0x97b8d2d7, 0x013cf656, 0x55cc0620,
0x714c83e3, 0x39c60858, 0x83df1a1e, 0x6c3ae3e5,
0xac76ec79, 0x0e5cabc1, 0xa9bc90a9, 0x519cb5fb,
0xdeb7b81f, 0x79b87677, 0x3f1fbb3c, 0x6e3ad8af,
0x9b830a54, 0xdd25aa2a, 0x6f9ffc5c, 0x85c6051f,
0xfd7a0c0a, 0x85bcb0f4, 0xd88fe5fb, 0xfa48e174,
0x1b1f53ce, 0x256f7e55, 0x540ab095, 0xc1c095e9,
0x639bcaaf, 0x47491ea6, 0x6f20033c, 0x1f68731c,
0x9b038088, 0xa73a9f7d, 0x54cf6b7a, 0x64e2d7bc,
0xeb292aa1, 0x8bbdfddf, 0x28837ace, 0xfb3939f1,
0x34e4b61e, 0xb6d26e06, 0x02e94f45, 0xcd4623f2,
0xb20b349e, 0xf14c5b54, 0x9a02ec7b, 0xbd1bbf9f,
0x50b31ad7, 0xf1e93229, 0xecdccec8, 0xf989fcec,
0xee9a1ffa, 0xaca5e9a6, 0xdfeb849e, 0x04baf520,
0x330473c6, 0x356d442f, 0x28f3f205, 0x86024a5a,
0xb8d1a0a0, 0xeec872d8, 0x14b9798c, 0x3a9c6fbc,
0x23ba6b7f, 0x0c93ddac, 0x8d4a7a2b, 0xd02036c7,
0x22988982, 0x3a4a0a16, 0x650396a7, 0x36c9b0d4,
0xe5b9794c, 0xdea6b1ad, 0x9fba8280, 0x41ee18ab,
0xa8a7ad1a, 0x21b10aca, 0x8cabed1f, 0x5176a68e,
0x1344e2d7, 0xdfc892da, 0x25b65754, 0xe7dd6eae,
0xcbaa0d89, 0x9c188be7, 0x8387e700, 0x64247562,
0xa97d6807, 0xe9a9440b, 0x4e1046a6, 0x605b7db1,
0x5260300c, 0xebd708d0, 0x1758ec9f, 0xe53bc887,
0x68f75c11, 0xf7dc9db4, 0xbb5c7b89, 0x8f0ba542,
0x40246ab5, 0x3191a9d9, 0xcebdd8e8, 0x7151df2e,
0x548556bc, 0x6e263fd2, 0x3de8b3a9, 0x6560258a,
0x118926c5, 0x113798ee, 0x2c8b2a2f, 0x7c64cd19,
0xc98e9eae, 0xe7983224, 0x854aee14, 0x9f0b618a,
0x85496f2b, 0x1067692e, 0xd342dc3a, 0xcda94c12,
0x637a6088, 0x6a75f70c, 0x69bf6396, 0xc51bd91b,
0xc689df99, 0x20f804eb, 0x9de43768, 0x5e117f5a,
0xaecf69a1, 0xea5b8567, 0x294ac70a, 0x4928700d,
0x94eb0da9, 0x3ac2f7ed, 0xbf619de3, 0x675d409f,
0xf7473152, 0x8f4f53da, 0x745a95de, 0x62b4d5a3,
0xc046f2c6, 0x7cf38d26, 0xad4be296, 0xecd0f66e,
0x7dca84a8, 0xc284845b, 0xcaf52e35, 0xe24c5697,
0xee131f7d, 0xf92b62d9, 0x145c2553, 0x48dd89a3,
0xf1252225, 0x144b3246, 0xd5bfc5a5, 0x1c81f2f0,
0x9c3f1185, 0xb3c084f1, 0xf7f7db81, 0x18f592a5,
0x945ca65b, 0xc7ad4305, 0x09696888, 0xe25a1941,
0xf68727bb, 0x0701e0f6, 0x8d6c51df, 0xe272bf32,
0x526dccb8, 0x5c5c67d8, 0x96c13c5d, 0x83a2d6ee,
0x66271a43, 0xe4f052c7, 0xb961a17d, 0xaf6e2804,
0x8602e17c, 0x980c3457, 0x82df30d4, 0x0ee36e90,
0xf1a4ee1e, 0xe975fff6, 0xb04c0dd3, 0x51f8baea,
0xe2a6a10b, 0x7ff1bd40, 0xd63d031a, 0xbdceaedc,
0xdf0dbc66, 0x669a5df6, 0x564a4840, 0x6aa87716,
0x2dfeadb1, 0x0beda2fb, 0x0c85a97a, 0xcbe14704,
0x42ca076a, 0x721fdb16, 0x2261ed75, 0xdf08b0c8,
0x91207a6a, 0x6c1283f8, 0x8e4d4b2c, 0xeb284b6e,
0xae6305ac, 0x6eb43439, 0xfcf751fd, 0x0afe400a,
0x96462e17, 0x8d463f29, 0x4bb65895, 0x908eae5f,
0xe847ead1, 0xb402c2aa, 0xa9f21d76, 0xc1863f44,
0x122c393e, 0xe90f6f7b, 0xd1d471de, 0xbdccbf26,
0xec699aca, 0xe25bceb7, 0x789c966d, 0x8d82f6e4,
0x0d5d71cf, 0xd08b5764, 0x84303c6b, 0xa425974d,
0x5c97e946, 0x5f406c45, 0x295c5d18, 0x33d06570,
0xc8265ea8, 0xc03c908e, 0x364316ab, 0x7a8bf153,
0x2926e6af, 0xbec17e2e, 0x4bcd538e, 0xbddb36d7,
0x3908cd37, 0x480c208a, 0x3e58acdc, 0x87529fa9,
0x555331f9, 0xbbbcd09f, 0x7d4c9513, 0x38c9f516,
0xb4abce71, 0x99184f0a, 0x7a0973f1, 0x0df52108,
0xe8f0dd3b, 0x5551c444, 0x60a63983, 0xc6e8b055,
0x234219e8, 0xd7811b13, 0x72ea7338, 0x6a3622b9,
0x840de83d, 0x7fbf60cc, 0x6d890139, 0xfdd2ba4e,
0xb983a9f9, 0xe74d06c7, 0x81afc1f1, 0x7380d443,
0x7ce3a2e9, 0xf8902bb1, 0xae9614ca, 0x187d41e7,
0x49481e82, 0x11db1056, 0xaedbc369, 0x053fac0d,
0xe8b6f7de, 0x9ad74c73, 0x2f255e13, 0x692ca69c,
0x0d288887, 0x121b2478, 0x947fdb20, 0xefb6cde1,
0x61789893, 0x863d9262, 0x5ddfe309, 0x1b4a6145,
0xe8e37363, 0xa09aebde, 0x2d77daeb, 0xacd1d814,
0x470c78bb, 0x0557d2b3, 0xe8444536, 0xe3ac0904,
0x0b6ae773, 0x8023717d, 0x5e0c213b, 0xde9d83b7,
0x22002947, 0x84659a92, 0x2826f5c9, 0x7ebbce23,
0x24823b12, 0x351d02f4, 0x5f91456b, 0x631a86f2,
0x7492365b, 0xb8a3ecb6, 0x23bc4de8, 0xaa2ed9e1,
0x3170be6f, 0x2b7c2418, 0x728be186, 0x29fc18db,
0x60068f0b, 0xd7360349, 0x64c5ac54, 0x91fa1620,
0x76d0a6a4, 0xcaf6fe5e, 0x77179ca6, 0x1b7f1236,
0xaa276c80, 0x573a1088, 0x26868cb9, 0xabdf5f81,
0xf33e24d7, 0x98062791, 0xdecae1aa, 0xb9b8c009,
0xea0a3c41, 0xb52eeb74, 0xdd2e96b2, 0x7ee942ff,
0x96908b97, 0x05e2946b, 0xa7e0617c, 0x04e775f5,
0xab40eff9, 0xac0cd1e7, 0x3a5ddb9d, 0xf8563a78,
0xb6f2fc3d, 0xaa4c2764, 0xf235e5e2, 0x15e26b2b,
0x2895d031, 0xf6da8ec3, 0x95c63d21, 0x69dd399a,
0x61df7118, 0xd2189f50, 0xc05dd0b6, 0x332c29f1,
0x64d7eaae, 0x29015cb5, 0xd7d99627, 0xecece841,
0x262277ee, 0x783426d5, 0xc5b001cb, 0x7884cdea,
0x07f4534f, 0xe25298a6, 0x5307babd, 0x18b7171a,
0xc41659a1, 0xf9be42db, 0x9f055809, 0xa23009ad,
0xd592a2ee, 0x9060ee77, 0xdc3a0b2a, 0xbdc986cb,
0xdb06dc13, 0x7cfb9295, 0xa7fc0afa, 0xedeb79da,
0x437741e6, 0x004484e8, 0x9d07049a, 0x06fc835d,
0xfcd08f22, 0xd7019c30, 0xd1395a3b, 0x640089ec,
0x06337dac, 0xd8564795, 0xa258982a, 0x7c5d84a3,
0xe1984dd3, 0xcfa1ff8a, 0x238f3a73, 0x99e25c62,
0xae3f9c75, 0x805b9461, 0xb510d84d, 0x64ba24b0,
0x883dbeae, 0x19fe8257, 0xa7121486, 0x2e2f4a6a,
0x5ea4da3e, 0xdf10d922, 0x964a1a7c, 0x40b9607f,
0x93b4b6e3, 0x13e512db, 0x5d626485, 0xf9d774e0,
0x16cf5074, 0x52367ae7, 0x468c8cb5, 0x10e2a05c,
0x1bc4f6b3, 0xff2c067e, 0x500bc7e9, 0x0cb1f50e,
0x24ab0230, 0x5fe2046a, 0x6191cffe, 0x67953574,
0x20bc2b1d, 0x218eee79, 0x583ae935, 0xd5af7a80,
0x11878b33, 0x210fbecf, 0x0e684f41, 0x1dd21e39,
0xd1438fa8, 0x6e741a29, 0x6da4aaaa, 0x09a6408f,
0x553e3629, 0x47af1ecd, 0x3d3322d8, 0xae04922c,
0x28da6139, 0x535013b5, 0xe3080780, 0x0aad3bb8,
0x66927173, 0x0c695f09, 0xa5397241, 0x938269c7,
0xbe5b1e04, 0x6c32f36b, 0x26302d0c, 0xf8d1fe43,
0xbd7a2654, 0xe45621cb, 0x91096d22, 0xaa9acbde,
0x10ae718a, 0xbfd80593, 0x5078a82b, 0xd60a5917,
0x9269c985, 0x3613cf74, 0x9ca5e594, 0x59341085,
0xfb957d8c, 0x70c37fcd, 0x2d208c9f, 0x3cf4c7f8,
0x37ca0753, 0xea6c78cf, 0xc0771644, 0x94d1e2e3,
0x887d8cdb, 0x79114340, 0x2c354af6, 0x71487e38,
0x48db4b8a, 0x6e0c6e35, 0x303e0ca2, 0xf0076e8e,
0x84c11fd0, 0x81e7c399, 0x4a096746, 0x32d80cc8,
0xd374210e, 0xf7fe2f72, 0xa060b42e, 0x716c69d4,
0x552a1770, 0x23372844, 0x0948b0c9, 0xc01bd8dc,
0xc2dd4b7b, 0x5e7b466a, 0x1335ab5d, 0x65d3b1ae,
0x3679f740, 0x9dba9ed4, 0x73d2650e, 0xfb42ff7c,
0x365b9522, 0x6d7ce5de, 0x3e53285e, 0x7d858210,
0xa1e293b3, 0x50a75b7f, 0x95814635, 0xaa841fc2,
0x9d89fc8c, 0xab31c2f1, 0x5b839378, 0x0b53ff16,
0xd89c93d0, 0x48b02f81, 0xfa91f450, 0x72a1faab,
0x35cd2ed9, 0x32d9cced, 0xf8ec09be, 0x17999f9b,
0xfbac48bb, 0x3e0aecfa, 0x474f7ce8, 0xac50ccb3,
0x2103a1df, 0xdd591aa7, 0xdb085b89, 0x12428434,
0xc0e8678e, 0xfdf8081b, 0x4271a683, 0x81320e16,
0xaf84ec26, 0x9e4d7ac2, 0x8ca58aa7, 0xdc01e974,
0x26feeb5d, 0x3b618041, 0xe7ccd56d, 0xf386a8f9,
0x423ed5d4, 0xb29994b6, 0x0ccad4a0, 0x1128d39a,
0x119613af, 0xb318e9f1, 0xbf8ccdef, 0xd4956cfe,
0xb0260bb2, 0x0790f9f5, 0x0b4d10f2, 0x1691106e,
0xfd90d946, 0x89b4d53b, 0x6860158d, 0x520bce00,
0x4fd635f4, 0xf9d28db5, 0xae0c2555, 0xbb68dded,
0xaab7b17d, 0xe2eaee59, 0x0576111c, 0xa5945d55,
0x281af6d8, 0xf81d4031, 0x89855b39, 0xe2019cfa,
0x08b1aff1, 0x3b9b1dbf, 0xe8936e6c, 0x4503b091,
0x4696cdc3, 0x005dfe4f, 0xa1c72def, 0x44f25715,
0x5f77a5e8, 0x1c3e47e4, 0x07df8741, 0xdd89f485,
0x73af8d19, 0x9725bead, 0x6914713a, 0xb69a273e,
0xf72b9813, 0xac503abe, 0x7d047d7c, 0x20463dd4,
0x32332d97, 0x58732620, 0x2aead994, 0x626dfb39,
0x2738074c, 0x9a8c5585, 0x26700be3, 0xe65f54ba,
0x400ae860, 0xccafd505, 0x3a72b5b7, 0x72e95665,
0xec080a3a, 0x401b9490, 0xab1a3cc6, 0x0c2047c7,
};

inline u32 GetRandom()
{
	static int randomIndex = 0;
	const u32 result = RANDOM[randomIndex++];
	randomIndex %= ArrayCount(RANDOM);
	return result;
}
