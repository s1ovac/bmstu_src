<!-- src/components/FileTree.svelte -->
<script>
    import { onMount } from 'svelte';
    import { writable, get } from 'svelte/store';
    import {
        getFileTree,
        getFolders,
        uploadFile,
        deleteFiles,
        downloadFile,
        createFolder,
        deleteFolder
    } from '../lib/api.js';

    const files = writable([]);
    const folders = writable([]);
    const error = writable(null);
    const selectedItems = writable([]); // Для хранения выбранных файлов и папок
    let uploadInput;
    let newFolderName = '';
    let currentFolderId = 0; // Корневая папка
    let folderStack = []; // Для отслеживания пути навигации

    onMount(async () => {
        await fetchData();
    });

    const fetchData = async () => {
        try {
            const token = JSON.parse(localStorage.getItem('user')).token;
            console.log(currentFolderId);
            const [folderResponse, fileResponse] = await Promise.all([
                getFolders(token, currentFolderId),
                getFileTree(token, currentFolderId)
            ]);
            folders.set(folderResponse.folders);
            files.set(fileResponse.files);
        } catch (err) {
            error.set(err.message);
        }
    };

    const handleUpload = async () => {
        const filesToUpload = uploadInput.files;
        if (filesToUpload.length === 0) {
            error.set('No file selected');
            return;
        }
        try {
            const token = JSON.parse(localStorage.getItem('user')).token;
            await uploadFile(token, currentFolderId, filesToUpload[0]);
            uploadInput.value = '';
            await fetchData();
        } catch (err) {
            error.set(err.message);
        }
    };

    const handleDelete = async () => {
        if (get(selectedItems).length === 0) {
            error.set('No items selected for deletion');
            return;
        }
        try {
            const token = JSON.parse(localStorage.getItem('user')).token;
            const fileIds = get(selectedItems).filter(item => item.type === 'file').map(item => item.id);
            const folderIds = get(selectedItems).filter(item => item.type === 'folder').map(item => item.id);

            if (fileIds.length > 0) {
                await deleteFiles(token, fileIds);
            }
            if (folderIds.length > 0) {
                for (let folderId of folderIds) {
                    await deleteFolder(token, folderId);
                }
            }
            selectedItems.set([]);
            await fetchData();
        } catch (err) {
            error.set(err.message);
        }
    };

    const handleDownload = async (fileId, fileName) => {
        try {
            const token = JSON.parse(localStorage.getItem('user')).token;
            const blob = await downloadFile(token, fileId);
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = fileName;
            document.body.appendChild(a);
            a.click();
            a.remove();
            window.URL.revokeObjectURL(url);
        } catch (err) {
            error.set(err.message);
        }
    };

    const handleCreateFolder = async () => {
        if (newFolderName.trim() === '') {
            error.set('Folder name cannot be empty');
            return;
        }
        try {
            const token = JSON.parse(localStorage.getItem('user')).token;
            await createFolder(token, newFolderName, currentFolderId);
            newFolderName = '';
            await fetchData();
        } catch (err) {
            error.set(err.message);
        }
    };

    const navigateToFolder = (folderId, folderName) => {
        folderStack.push({ id: currentFolderId, name: folderName });
        currentFolderId = folderId;
        fetchData();
    };

    const navigateBack = () => {
        if (folderStack.length > 0) {
            const previousFolder = folderStack.pop();
            currentFolderId = previousFolder.id;
            fetchData();
        }
    };

    const toggleItemSelection = (item) => {
        const index = get(selectedItems).findIndex(selected => selected.id === item.id && selected.type === item.type);
        if (index !== -1) {
            selectedItems.update(items => items.filter((_, i) => i !== index));
        } else {
            selectedItems.update(items => [...items, item]);
        }
    };
</script>

<div class="actions">
    <input type="file" bind:this={uploadInput} />
    <button on:click={handleUpload}>Загрузить файл</button>

    <input
            type="text"
            placeholder="Имя новой папки"
            bind:value={newFolderName}
            class="folderInput"
    />
    <button on:click={handleCreateFolder}>Создать папку</button>

    <button on:click={handleDelete}>Удалить выбранное</button>
    {#if folderStack.length > 0}
        <button on:click={navigateBack}>Назад</button>
    {/if}
</div>

{#if $error}
    <div class="error">{$error}</div>
{/if}

<div class="breadcrumb">
    <span on:click={() => { currentFolderId = 0; folderStack = []; fetchData(); }}>Мой диск</span>
    {#each folderStack as folder}
        <span> / </span>
        <span on:click={() => {
            currentFolderId = folder.id;
            folderStack = folderStack.slice(0, folderStack.indexOf(folder));
            fetchData();
        }}>{folder.name}</span>
    {/each}
</div>

<div class="fileTree">
    {#if $folders.length === 0 && $files.length === 0}
        <div class="emptyMessage">Эта папка пуста</div>
    {/if}

    {#if $folders.length > 0}
        <div class="folderSection">
            <h3>Папки</h3>
            <ul>
                {#each $folders as folder}
                    <li>
                        <input
                                type="checkbox"
                                on:change={() => toggleItemSelection({ id: folder.folder_id, type: 'folder' })}
                        />
                        <i class="material-icons folderIcon" on:click={() => navigateToFolder(folder.folder_id, folder.folder_name)}>folder</i>
                        <span on:click={() => navigateToFolder(folder.folder_id, folder.folder_name)}>{folder.folder_name}</span>
                    </li>
                {/each}
            </ul>
        </div>
    {/if}

    {#if $files.length > 0}
        <div class="fileSection">
            <h3>Файлы</h3>
            <ul>
                {#each $files as file}
                    <li>
                        <input
                                type="checkbox"
                                on:change={() => toggleItemSelection({ id: file.file_id, type: 'file' })}
                        />
                        <i class="material-icons fileIcon">insert_drive_file</i>
                        <span>{file.file_name}</span>
                        <button on:click={() => handleDownload(file.file_id, file.file_name)}>Скачать</button>
                    </li>
                {/each}
            </ul>
        </div>
    {/if}
</div>

<style>
    @import url('https://fonts.googleapis.com/icon?family=Material+Icons');

    .actions {
        display: flex;
        align-items: center;
        margin-bottom: 20px;
    }

    .actions input[type="file"],
    .actions .folderInput {
        margin-right: 10px;
        padding: 5px;
    }

    .actions button {
        margin-right: 10px;
        padding: 8px 12px;
        background-color: #4285f4;
        color: white;
        border: none;
        cursor: pointer;
        border-radius: 4px;
    }

    .actions button:hover {
        background-color: #3367d6;
    }

    .error {
        color: red;
        margin-top: 10px;
    }

    .breadcrumb {
        margin-bottom: 20px;
        font-size: 16px;
    }

    .breadcrumb span {
        cursor: pointer;
        color: #4285f4;
    }

    .breadcrumb span:hover {
        text-decoration: underline;
    }

    .fileTree {
        display: flex;
        flex-direction: column;
    }

    .folderSection,
    .fileSection {
        margin-bottom: 20px;
    }

    .folderSection h3,
    .fileSection h3 {
        margin-bottom: 10px;
    }

    ul {
        list-style-type: none;
        padding: 0;
    }

    li {
        display: flex;
        align-items: center;
        margin-bottom: 10px;
    }

    li input[type="checkbox"] {
        margin-right: 10px;
    }

    .folderIcon,
    .fileIcon {
        margin-right: 10px;
        cursor: pointer;
        color: #5f6368;
    }

    .folderIcon:hover,
    .fileIcon:hover {
        color: #4285f4;
    }

    li span {
        flex-grow: 1;
        cursor: pointer;
    }

    li span:hover {
        text-decoration: underline;
    }

    li button {
        padding: 5px 10px;
        background-color: #4285f4;
        color: white;
        border: none;
        cursor: pointer;
        border-radius: 4px;
    }

    li button:hover {
        background-color: #3367d6;
    }

    .emptyMessage {
        font-size: 18px;
        color: #5f6368;
        text-align: center;
        margin-top: 50px;
    }
</style>
