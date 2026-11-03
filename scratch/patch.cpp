        try {
            nlohmann::json wrapper = nlohmann::json::from_msgpack(bytes);
            if (wrapper.contains("MigrationRoot") && wrapper["MigrationRoot"].is_string()) {
                nlohmann::json root = nlohmann::json::parse(wrapper["MigrationRoot"].get<std::string>());
                return Earthcall::Storage::MigrationFramework::migrateLegacySave(root);
            }
            return Earthcall::Storage::MigrationFramework::migrateLegacySave(wrapper);
        } catch (...) {
            // It might be a plain JSON file from Phase 3 before ecform became strictly msgpack
            try {
                nlohmann::json j = nlohmann::json::parse(bytes);
                return Earthcall::Storage::MigrationFramework::migrateLegacySave(j);
            } catch (const std::exception& e) {
                std::cerr << "[SaveSystem] Malformed msgpack ecform (and not valid JSON either) in: " << filepath << "\n";
                return nlohmann::json();
            }
        }
