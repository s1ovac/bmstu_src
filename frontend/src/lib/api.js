// src/lib/api.js

// Используем переменные окружения
const AUTH_BASE_URL = 'http://localhost:8082';
const FILE_BASE_URL = 'http://localhost:8081';

// Функции аутентификации используют AUTH_BASE_URL
export const login = async (email, password) => {
    const response = await fetch(`${AUTH_BASE_URL}/api/v1/login`, {
        method: "POST",
        headers: {
            "Content-Type": "application/json",
        },
        body: JSON.stringify({ "login": email, "password": password }),
    });

    if (response.status === 404) {
        throw new Error("User not found");
    }

    if (response.status === 401) {
        throw new Error("Invalid credentials");
    }

    if (!response.ok) {
        const errorResponse = await response.json();
        console.error(errorResponse);
        throw new Error('Login failed');
    }

    return response.json();
};

export const signup = async (email, password) => {
    const response = await fetch(`${AUTH_BASE_URL}/api/v1/signup`, {
        method: "POST",
        headers: {
            "Content-Type": "application/json",
        },
        body: JSON.stringify({ "login": email, "password": password }),
    });

    if (response.status === 409) {
        throw new Error("User already exists");
    }

    if (!response.ok) {
        console.error(await response.json());
        throw new Error('Signup failed');
    }

    return response.json();
};

// Файловые операции используют FILE_BASE_URL

// Функция для получения списка файлов
export const getFileTree = async (token, folderId = 0) => {
    try {
        console.log(`Fetching files for folder ID: ${folderId}`);

        // Защита от некорректных значений folderId
        const sanitizedFolderId = folderId !== null && folderId !== undefined ? folderId : 0;

        const response = await fetch(`${FILE_BASE_URL}/api/v1/files?folder_id=${sanitizedFolderId}`, {
            headers: {
                'Authorization': `Bearer ${token}`
            }
        });

        if (!response.ok) {
            const errorText = await response.text();
            console.error(`Failed to fetch files: ${response.status} ${response.statusText}`, errorText);
            throw new Error('Failed to fetch file tree');
        }

        const data = await response.json();
        console.log('Files response data:', data);

        // Убедимся, что всегда возвращаем объект с массивом files
        return {
            files: Array.isArray(data.files) ? data.files : []
        };
    } catch (error) {
        console.error('Error in getFileTree:', error);
        throw error;
    }
};

export const uploadFile = async (token, folderId, file) => {
    const formData = new FormData();
    formData.append('file', file);

    const response = await fetch(`${FILE_BASE_URL}/api/v1/files?folder_id=${folderId}`, {
        method: 'POST',
        headers: {
            'Authorization': `Bearer ${token}`
        },
        body: formData
    });

    if (!response.ok) {
        throw new Error('Failed to upload file');
    }

    return await response.json();
};

// New folder operations
export const getFolders = async (token, parentFolderId = 0) => {
    try {
        console.log(`Fetching folders for parent folder ID: ${parentFolderId}`);

        // Защита от некорректных значений parentFolderId
        const sanitizedParentFolderId = parentFolderId !== null && parentFolderId !== undefined ? parentFolderId : 0;

        const response = await fetch(`${FILE_BASE_URL}/api/v1/folders?parent_folder_id=${sanitizedParentFolderId}`, {
            headers: {
                'Authorization': `Bearer ${token}`
            }
        });

        if (!response.ok) {
            const errorText = await response.text();
            console.error(`Failed to fetch folders: ${response.status} ${response.statusText}`, errorText);
            throw new Error('Failed to fetch folders');
        }

        const data = await response.json();
        console.log('Folders response data:', data);

        // Убедимся, что всегда возвращаем объект с массивом folders
        return {
            folders: Array.isArray(data.folders) ? data.folders : []
        };
    } catch (error) {
        console.error('Error in getFolders:', error);
        throw error;
    }
};

export const createFolder = async (token, folderName, parentFolderId = 0) => {
    const response = await fetch(`${FILE_BASE_URL}/api/v1/folders`, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
            'Authorization': `Bearer ${token}`
        },
        body: JSON.stringify({
            folder_name: folderName,
            parent_folder_id: parentFolderId
        })
    });

    if (!response.ok) {
        throw new Error('Failed to create folder');
    }

    return await response.json();
};

export const deleteFolder = async (token, folderId) => {
    const response = await fetch(`${FILE_BASE_URL}/api/v1/folders/${folderId}`, {
        method: 'DELETE',
        headers: {
            'Authorization': `Bearer ${token}`
        }
    });

    if (!response.ok) {
        throw new Error('Failed to delete folder');
    }

    return true;
};

export const moveFile = async (token, fileId, targetFolderId) => {
    const response = await fetch(`${FILE_BASE_URL}/api/v1/file/move`, {
        method: 'PUT',
        headers: {
            'Content-Type': 'application/json',
            'Authorization': `Bearer ${token}`
        },
        body: JSON.stringify({
            file_id: fileId,
            target_folder_id: targetFolderId
        })
    });

    if (!response.ok) {
        throw new Error('Failed to move file');
    }

    return await response.json();
};

export const moveFiles = async (token, fileIds, targetFolderId) => {
    const response = await fetch(`${FILE_BASE_URL}/api/v1/files/move`, {
        method: 'PUT',
        headers: {
            'Content-Type': 'application/json',
            'Authorization': `Bearer ${token}`
        },
        body: JSON.stringify({
            file_ids: fileIds,
            target_folder_id: targetFolderId
        })
    });

    if (!response.ok) {
        throw new Error('Failed to move files');
    }

    return await response.json();
};

export const deleteFiles = async (token, fileIds) => {
    const response = await fetch(`${FILE_BASE_URL}/api/v1/files`, {
        method: 'DELETE',
        headers: {
            'Content-Type': 'application/json',
            'Authorization': `Bearer ${token}`
        },
        body: JSON.stringify({ file_ids: fileIds })
    });

    if (!response.ok) {
        throw new Error('Failed to delete files');
    }

    return true;
};

export const downloadFile = async (token, fileId) => {
    const response = await fetch(`${FILE_BASE_URL}/api/v1/file?file_id=${fileId}`, {
        headers: {
            'Authorization': `Bearer ${token}`
        }
    });

    if (!response.ok) {
        throw new Error('Failed to download file');
    }

    return await response.blob();
};

export const getAllUsers = async (token) => {
    const response = await fetch(`${AUTH_BASE_URL}/api/v1/admin/users`, {
        headers: {
            "Authorization": `Bearer ${token}`
        }
    });

    if (!response.ok) {
        throw new Error("Failed to fetch all users");
    }

    return await response.json();
};

export const getAllGroups = async (token) => {
    const response = await fetch(`${AUTH_BASE_URL}/api/v1/groups`, {
        headers: {
            "Authorization": `Bearer ${token}`
        }
    });

    if (!response.ok) {
        throw new Error("Failed to fetch all groups");
    }

    return await response.json();
};

export const createGroup = async (token, groupName) => {
    const response = await fetch(`${AUTH_BASE_URL}/api/v1/groups`, {
        method: "POST",
        headers: {
            "Content-Type": "application/json",
            "Authorization": `Bearer ${token}`
        },
        body: JSON.stringify({ group_name: groupName })
    });

    if (!response.ok) {
        throw new Error("Failed to create group");
    }

    // бэкенд возвращает JSON с message = "Group created successfully"
    return await response.json();
};

export const renameGroup = async (token, groupId, newName) => {
    const response = await fetch(`${AUTH_BASE_URL}/api/v1/groups/${groupId}`, {
        method: "PUT",
        headers: {
            "Content-Type": "application/json",
            "Authorization": `Bearer ${token}`
        },
        body: JSON.stringify({ new_name: newName })
    });

    if (!response.ok) {
        throw new Error("Failed to rename group");
    }

    return await response.json();
};

export const deleteGroup = async (token, groupId) => {
    const response = await fetch(`${AUTH_BASE_URL}/api/v1/groups/${groupId}`, {
        method: "DELETE",
        headers: {
            "Authorization": `Bearer ${token}`
        }
    });

    if (!response.ok) {
        throw new Error("Failed to delete group");
    }

    return await response.json();
};

export const addUserToGroup = async (token, userId, groupId) => {
    // POST /api/v1/groups/{group_id}/add/{user_id}
    const url = `${AUTH_BASE_URL}/api/v1/groups/${groupId}/add/${userId}`;
    const response = await fetch(url, {
        method: "POST",
        headers: {
            "Authorization": `Bearer ${token}`
        }
    });

    if (!response.ok) {
        throw new Error("Failed to add user to group");
    }

    return await response.json();
};

export const removeUserFromGroup = async (token, userId, groupId) => {
    // DELETE /api/v1/groups/{group_id}/remove/{user_id}
    const url = `${AUTH_BASE_URL}/api/v1/groups/${groupId}/remove/${userId}`;
    const response = await fetch(url, {
        method: "DELETE",
        headers: {
            "Authorization": `Bearer ${token}`
        }
    });

    if (!response.ok) {
        throw new Error("Failed to remove user from group");
    }

    return await response.json();
};

export const getUserRoles = async (token, userId) => {
    const response = await fetch(`${AUTH_BASE_URL}/api/v1/users/${userId}/roles`, {
        headers: {
            "Authorization": `Bearer ${token}`
        }
    });

    if (!response.ok) {
        throw new Error("Failed to fetch user roles");
    }

    return await response.json();
};

// Получение системной статистики
export const getSystemStats = async (token) => {
    try {
        const response = await fetch(`${FILE_BASE_URL}/api/v1/admin/stats`, {
            headers: {
                'Authorization': `Bearer ${token}`
            }
        });

        if (!response.ok) {
            const errorText = await response.text();
            console.error(`Failed to fetch system stats: ${response.status} ${response.statusText}`, errorText);
            throw new Error('Failed to fetch system statistics');
        }

        return await response.json();
    } catch (error) {
        console.error('Error in getSystemStats:', error);
        throw error;
    }
};

// Получение содержимого пользователя (файлы и папки)
export const getUserContent = async (token, userId) => {
    try {
        const response = await fetch(`${FILE_BASE_URL}/api/v1/admin/users/${userId}/content`, {
            headers: {
                'Authorization': `Bearer ${token}`
            }
        });

        if (!response.ok) {
            const errorText = await response.text();
            console.error(`Failed to fetch user content: ${response.status} ${response.statusText}`, errorText);
            throw new Error(`Failed to fetch content for user ${userId}`);
        }

        return await response.json();
    } catch (error) {
        console.error(`Error in getUserContent for user ${userId}:`, error);
        throw error;
    }
};

// Получение всех файлов в системе
export const getAllFilesAdmin = async (token) => {
    try {
        const response = await fetch(`${FILE_BASE_URL}/api/v1/admin/files`, {
            headers: {
                'Authorization': `Bearer ${token}`
            }
        });

        if (!response.ok) {
            const errorText = await response.text();
            console.error(`Failed to fetch all files: ${response.status} ${response.statusText}`, errorText);
            throw new Error('Failed to fetch all files');
        }

        return await response.json();
    } catch (error) {
        console.error('Error in getAllFilesAdmin:', error);
        throw error;
    }
};

// Получение всех папок в системе
export const getAllFoldersAdmin = async (token) => {
    try {
        const response = await fetch(`${FILE_BASE_URL}/api/v1/admin/folders`, {
            headers: {
                'Authorization': `Bearer ${token}`
            }
        });

        if (!response.ok) {
            const errorText = await response.text();
            console.error(`Failed to fetch all folders: ${response.status} ${response.statusText}`, errorText);
            throw new Error('Failed to fetch all folders');
        }

        return await response.json();
    } catch (error) {
        console.error('Error in getAllFoldersAdmin:', error);
        throw error;
    }
};

export const changePassword = async (token, currentPassword, newPassword) => {
    try {
        const response = await fetch(`${AUTH_BASE_URL}/api/v1/change-password`, {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
                "Authorization": `Bearer ${token}`
            },
            body: JSON.stringify({
                currentPassword,
                newPassword
            })
        });

        if (response.status === 401) {
            throw new Error("Неверный текущий пароль");
        }

        if (response.status === 400) {
            const data = await response.json();
            throw new Error(data.error || "Некорректные данные");
        }

        if (!response.ok) {
            throw new Error("Ошибка при смене пароля");
        }

        return await response.json();
    } catch (error) {
        console.error("Password change error:", error);
        throw error;
    }
};