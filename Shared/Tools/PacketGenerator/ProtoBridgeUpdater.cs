using System.Text;
using System.Xml;
using System.Xml.Linq;

namespace PacketGenerator
{
    internal static class ProtoBridgeUpdater
    {
        private static readonly HashSet<string> BaseProtoFiles = new()
        {
            "Enum", "Struct", "PacketOption"
        };

        public static void UpdateVcxproj(
            string vcxprojPath,
            List<HandlerInfo> handlers,
            string clientRole)
        {
            if (string.IsNullOrEmpty(vcxprojPath) || !File.Exists(vcxprojPath))
            {
                return;
            }

            // 클라이언트가 사용하는 proto 파일 수집
            var usedProtoFiles = new HashSet<string>(BaseProtoFiles);

            foreach (var handler in handlers)
            {
                foreach (var packet in handler.Packets)
                {
                    if (packet.Receivers.Any(r => RoleHelper.IsMatchRole(r, clientRole)) ||
                        RoleHelper.IsMatchRole(packet.Sender, clientRole))
                    {
                        usedProtoFiles.Add(handler.ProtoFileName);
                    }
                }
            }

            Console.WriteLine($"[ProtoBridge] Used proto files: {string.Join(", ", usedProtoFiles)}");

            // vcxproj XML 파싱
            var doc = XDocument.Load(vcxprojPath);
            XNamespace ns = "http://schemas.microsoft.com/developer/msbuild/2003";

            // 기존 pb.cc ClCompile과 pb.h ClInclude 제거
            var compileGroup = doc.Descendants(ns + "ClCompile")
                .Where(e => e.Attribute("Include")?.Value.Contains(".pb.cc") == true)
                .ToList();

            var includeGroup = doc.Descendants(ns + "ClInclude")
                .Where(e => e.Attribute("Include")?.Value.Contains(".pb.h") == true)
                .ToList();

            foreach (var elem in compileGroup)
            {
                elem.Remove();
            }

            foreach (var elem in includeGroup)
            {
                elem.Remove();
            }

            // 새로운 pb.cc 항목 추가
            var compileItemGroup = doc.Descendants(ns + "ItemGroup")
                .FirstOrDefault(ig => ig.Elements(ns + "ClCompile").Any());

            if (compileItemGroup == null)
            {
                return;
            }

            // pch.cpp 앞에 삽입
            var pchCompile = compileItemGroup.Elements(ns + "ClCompile")
                .FirstOrDefault(e => e.Attribute("Include")?.Value == "pch.cpp");

            foreach (var protoFile in usedProtoFiles.OrderBy(f => f))
            {
                var relativePath = $@"..\..\Shared\Protocol\{protoFile}.pb.cc";

                var compileElement = new XElement(ns + "ClCompile",
                    new XAttribute("Include", relativePath),
                    new XElement(ns + "PrecompiledHeader",
                        new XAttribute("Condition", "'$(Configuration)|$(Platform)'=='Debug|Win32'"), "NotUsing"),
                    new XElement(ns + "PrecompiledHeader",
                        new XAttribute("Condition", "'$(Configuration)|$(Platform)'=='Release|Win32'"), "NotUsing"),
                    new XElement(ns + "PrecompiledHeader",
                        new XAttribute("Condition", "'$(Configuration)|$(Platform)'=='Debug|x64'"), "NotUsing"),
                    new XElement(ns + "PrecompiledHeader",
                        new XAttribute("Condition", "'$(Configuration)|$(Platform)'=='Release|x64'"), "NotUsing")
                );

                if (pchCompile != null)
                {
                    pchCompile.AddBeforeSelf(compileElement);
                }
                else
                {
                    compileItemGroup.Add(compileElement);
                }
            }

            // 새로운 pb.h 항목 추가
            var includeItemGroup = doc.Descendants(ns + "ItemGroup")
                .FirstOrDefault(ig => ig.Elements(ns + "ClInclude").Any());

            if (includeItemGroup == null)
            {
                return;
            }

            var pchInclude = includeItemGroup.Elements(ns + "ClInclude")
                .FirstOrDefault(e => e.Attribute("Include")?.Value == "pch.h");

            foreach (var protoFile in usedProtoFiles.OrderBy(f => f))
            {
                var relativePath = $@"..\..\Shared\Protocol\{protoFile}.pb.h";

                var includeElement = new XElement(ns + "ClInclude",
                    new XAttribute("Include", relativePath));

                if (pchInclude != null)
                {
                    pchInclude.AddBeforeSelf(includeElement);
                }
                else
                {
                    includeItemGroup.Add(includeElement);
                }
            }

            // 기존 .proto None 항목 제거 후 재생성
            var noneItems = doc.Descendants(ns + "None")
                .Where(e => e.Attribute("Include")?.Value.Contains(".proto") == true)
                .ToList();

            foreach (var elem in noneItems)
            {
                elem.Remove();
            }

            var noneItemGroup = doc.Descendants(ns + "ItemGroup")
                .FirstOrDefault(ig => ig.Elements(ns + "None").Any());

            if (noneItemGroup == null)
            {
                noneItemGroup = new XElement(ns + "ItemGroup");
                doc.Root?.Add(noneItemGroup);
            }

            foreach (var protoFile in usedProtoFiles.OrderBy(f => f))
            {
                noneItemGroup.Add(new XElement(ns + "None",
                    new XAttribute("Include", $@"..\..\Shared\Protocol\Proto\{protoFile}.proto")));
            }

            // 저장
            var settings = new XmlWriterSettings
            {
                Indent = true,
                IndentChars = "  ",
                Encoding = new UTF8Encoding(true)
            };

            using var writer = XmlWriter.Create(vcxprojPath, settings);
            doc.Save(writer);

            Console.WriteLine($"[ProtoBridge] Updated: {vcxprojPath}");

            // filters 파일도 업데이트
            var filtersPath = vcxprojPath + ".filters";
            if (File.Exists(filtersPath))
            {
                UpdateFilters(filtersPath, usedProtoFiles);
            }
        }

        private static void UpdateFilters(string filtersPath, HashSet<string> usedProtoFiles)
        {
            var doc = XDocument.Load(filtersPath);
            XNamespace ns = "http://schemas.microsoft.com/developer/msbuild/2003";

            // 기존 pb.cc, pb.h, .proto 항목 제거 (pch 제외)
            var compileItems = doc.Descendants(ns + "ClCompile")
                .Where(e => e.Attribute("Include")?.Value.Contains(".pb.cc") == true)
                .ToList();
            var includeItems = doc.Descendants(ns + "ClInclude")
                .Where(e => e.Attribute("Include")?.Value.Contains(".pb.h") == true)
                .ToList();
            var noneItems = doc.Descendants(ns + "None")
                .Where(e => e.Attribute("Include")?.Value.Contains(".proto") == true)
                .ToList();

            foreach (var elem in compileItems) { elem.Remove(); }
            foreach (var elem in includeItems) { elem.Remove(); }
            foreach (var elem in noneItems) { elem.Remove(); }

            // ClCompile ItemGroup 찾기
            var compileGroup = doc.Descendants(ns + "ItemGroup")
                .FirstOrDefault(ig => ig.Elements(ns + "ClCompile").Any());
            var includeGroup = doc.Descendants(ns + "ItemGroup")
                .FirstOrDefault(ig => ig.Elements(ns + "ClInclude").Any());
            var noneGroup = doc.Descendants(ns + "ItemGroup")
                .FirstOrDefault(ig => ig.Elements(ns + "None").Any());

            // None ItemGroup이 없으면 새로 생성
            if (noneGroup == null)
            {
                noneGroup = new XElement(ns + "ItemGroup");
                doc.Root?.Add(noneGroup);
            }

            foreach (var protoFile in usedProtoFiles.OrderBy(f => f))
            {
                // .pb.cc
                compileGroup?.Add(new XElement(ns + "ClCompile",
                    new XAttribute("Include", $@"..\..\Shared\Protocol\{protoFile}.pb.cc"),
                    new XElement(ns + "Filter", "Network\\Protocol")));

                // .pb.h
                includeGroup?.Add(new XElement(ns + "ClInclude",
                    new XAttribute("Include", $@"..\..\Shared\Protocol\{protoFile}.pb.h"),
                    new XElement(ns + "Filter", "Network\\Protocol")));

                // .proto
                noneGroup.Add(new XElement(ns + "None",
                    new XAttribute("Include", $@"..\..\Shared\Protocol\Proto\{protoFile}.proto"),
                    new XElement(ns + "Filter", "Network\\Protocol\\Proto")));
            }

            var settings = new XmlWriterSettings
            {
                Indent = true,
                IndentChars = "  ",
                Encoding = new UTF8Encoding(true)
            };

            using var writer = XmlWriter.Create(filtersPath, settings);
            doc.Save(writer);

            Console.WriteLine($"[ProtoBridge] Updated filters: {filtersPath}");
        }

        public static void CopyClientProtocolHeaders(
            string protoSourceDir,
            string clientProtocolDir,
            List<HandlerInfo> handlers,
            string clientRole)
        {
            if (string.IsNullOrEmpty(clientProtocolDir))
            {
                return;
            }

            if (!Directory.Exists(clientProtocolDir))
            {
                Directory.CreateDirectory(clientProtocolDir);
            }

            // 클라이언트가 사용하는 proto 파일 수집
            var usedProtoFiles = new HashSet<string>(BaseProtoFiles);

            foreach (var handler in handlers)
            {
                foreach (var packet in handler.Packets)
                {
                    if (packet.Receivers.Any(r => RoleHelper.IsMatchRole(r, clientRole)) ||
                        RoleHelper.IsMatchRole(packet.Sender, clientRole))
                    {
                        usedProtoFiles.Add(handler.ProtoFileName);
                    }
                }
            }

            // 기존 복사본 삭제 (클라 모듈이 Shared의 생성물을 직접 컴파일)
            foreach (var file in Directory.GetFiles(clientProtocolDir, "*.pb.h"))
            {
                File.Delete(file);
            }

            foreach (var file in Directory.GetFiles(clientProtocolDir, "*.pb.cpp"))
            {
                File.Delete(file);
            }

            foreach (var file in Directory.GetFiles(clientProtocolDir, "*.pb.cc"))
            {
                File.Delete(file);
            }

            var copiedSources = 0;
            foreach (var protoFile in usedProtoFiles)
            {
                var sourceHeader = Path.Combine(protoSourceDir, $"{protoFile}.pb.h");
                if (File.Exists(sourceHeader))
                {
                    File.Copy(sourceHeader, Path.Combine(clientProtocolDir, $"{protoFile}.pb.h"), true);
                }

                // .pb.cc → .pb.cpp : UBT가 모듈 트리에서 .cpp를 스캔하며, ProtoBridge.lib와 이중 링크 방지
                var sourceCc = Path.Combine(protoSourceDir, $"{protoFile}.pb.cc");
                if (File.Exists(sourceCc))
                {
                    File.Copy(sourceCc, Path.Combine(clientProtocolDir, $"{protoFile}.pb.cpp"), true);
                    copiedSources++;
                }
            }

            Console.WriteLine($"[ClientProtocol] Copied {usedProtoFiles.Count} headers and {copiedSources} generated sources to: {clientProtocolDir}");
        }
    }
}
