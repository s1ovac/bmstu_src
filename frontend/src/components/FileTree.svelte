<!-- src/components/FileTree.svelte -->
<script>
    import { onMount } from 'svelte';
    import { writable, derived, get } from 'svelte/store';
    import {
        getFileTree,
        getFolders,
        uploadFile,
        deleteFiles,
        downloadFile,
        createFolder,
        deleteFolder,
        moveFile,
        moveFiles,
        getUserGroups,
        createSharedFolder,
        uploadSharedFile,
        getAllFavorites,
        toggleFileFavorite,
        toggleFolderFavorite,
    } from '../lib/api.js';

    export let sharedMode = false;
    export let favoritesMode = false;

    // Основные хранилища данных
    const foldersList = writable([]);
    const filesList = writable([]);
    const error = writable(null);
    const success = writable(null);
    const selectedItems = writable([]);
    const loading = writable(false);
    const searchQuery = writable('');

    // Локальные переменные
    let uploadInput;
    let newFolderName = '';
    let currentFolderId = 0;
    let folderStack = [];
    let showNewFolderDialog = false;
    let viewMode = localStorage.getItem('viewMode') || 'grid';
    let isUploading = false;
    let showDeleteConfirm = false;
    let dragOver = false;
    let showMoveDialog = false;
    let allFolders = writable([]);
    let targetFolderId = 0;
    let isFoldersLoading = false;
    let userGroups = writable([]);
    let selectedGroupId = null;

    // Функция для переключения избранного
    const toggleFavorite = async (item, type, event) => {
        if (event) event.stopPropagation();

        try {
            const token = JSON.parse(localStorage.getItem('user')).token;
            const isCurrentlyFavorite = item.is_favorite || false;

            if (type === 'file') {
                await toggleFileFavorite(token, item.file_id, !isCurrentlyFavorite);
            } else {
                await toggleFolderFavorite(token, item.folder_id, !isCurrentlyFavorite);
            }

            success.set(isCurrentlyFavorite ? 'Удалено из избранного' : 'Добавлено в избранное');
            setTimeout(() => success.set(null), 3000);

            // Обновляем данные
            await fetchData();
        } catch (err) {
            error.set('Ошибка при изменении избранного: ' + err.message);
            setTimeout(() => error.set(null), 3000);
        }
    };

    // Функция для загрузки всех папок (для диалога перемещения)
    const fetchAllFolders = async () => {
        isFoldersLoading = true;
        try {
            const token = JSON.parse(localStorage.getItem('user')).token;
            const rootFoldersResponse = await getFolders(token, 0);
            let folders = rootFoldersResponse.folders || [];

            const subFolderPromises = folders.map(async folder => {
                const subFoldersResponse = await getFolders(token, folder.folder_id);
                return {
                    ...folder,
                    subFolders: subFoldersResponse.folders || []
                };
            });

            const foldersWithSubs = await Promise.all(subFolderPromises);
            allFolders.set(foldersWithSubs);
        } catch (err) {
            console.error('Error fetching folders:', err);
            error.set('Не удалось загрузить список папок');
            setTimeout(() => error.set(null), 3000);
        } finally {
            isFoldersLoading = false;
        }
    };

    // Функция для открытия диалога перемещения
    const openMoveDialog = async () => {
        if ($selectedItems.length === 0) {
            error.set('Не выбраны элементы для перемещения');
            setTimeout(() => error.set(null), 3000);
            return;
        }

        const onlyFolders = $selectedItems.every(item => item.type === 'folder');
        if (onlyFolders) {
            error.set('Перемещение папок в текущей версии не поддерживается');
            setTimeout(() => error.set(null), 3000);
            return;
        }

        await fetchAllFolders();
        targetFolderId = 0;
        showMoveDialog = true;
    };

    const handleMove = async () => {
        if ($selectedItems.length === 0) return;

        loading.set(true);
        showMoveDialog = false;

        try {
            const token = JSON.parse(localStorage.getItem('user')).token;
            const fileItems = $selectedItems.filter(item => item.type === 'file');

            if (fileItems.length === 1) {
                await moveFile(token, fileItems[0].id, targetFolderId);
            } else if (fileItems.length > 1) {
                const fileIds = fileItems.map(item => item.id);
                await moveFiles(token, fileIds, targetFolderId);
            }

            success.set('Файлы успешно перемещены');
            setTimeout(() => success.set(null), 3000);
            selectedItems.set([]);
            await fetchData();
        } catch (err) {
            error.set('Ошибка при перемещении файлов: ' + err.message);
            setTimeout(() => error.set(null), 3000);
        } finally {
            loading.set(false);
        }
    };

    const filteredFolders = derived(
        [foldersList, searchQuery],
        ([$foldersList, $searchQuery]) => {
            if (!$searchQuery || $searchQuery.trim() === '') {
                return $foldersList;
            }

            const query = $searchQuery.trim().toLowerCase();
            return $foldersList.filter(folder =>
                folder && folder.folder_name &&
                folder.folder_name.toLowerCase().includes(query)
            );
        }
    );

    const filteredFiles = derived(
        [filesList, searchQuery],
        ([$filesList, $searchQuery]) => {
            if (!$searchQuery || $searchQuery.trim() === '') {
                return $filesList;
            }

            const query = $searchQuery.trim().toLowerCase();
            return $filesList.filter(
                file => file && file.file_name &&
                    file.file_name.toLowerCase().includes(query)
            );
        }
    );

    // Функция для форматирования размера файла
    const formatFileSize = (size) => {
        if (size < 1024) return `${size} B`;
        else if (size < 1024 * 1024) return `${(size / 1024).toFixed(1)} KB`;
        else if (size < 1024 * 1024 * 1024) return `${(size / (1024 * 1024)).toFixed(1)} MB`;
        else return `${(size / (1024 * 1024 * 1024)).toFixed(1)} GB`;
    };

    // Функция для получения даты в удобном формате
    const formatDate = (dateString) => {
        const date = new Date(dateString);
        return date.toLocaleDateString('ru-RU', {
            day: '2-digit',
            month: '2-digit',
            year: 'numeric'
        });
    };

    // Функция для определения иконки файла
    const getFileIcon = (fileName) => {
        const extension = fileName.split('.').pop().toLowerCase();

        const iconMap = {
            'pdf': 'picture_as_pdf',
            'doc': 'description',
            'docx': 'description',
            'txt': 'article',
            'xls': 'table_chart',
            'xlsx': 'table_chart',
            'ppt': 'slideshow',
            'pptx': 'slideshow',
            'jpg': 'image',
            'jpeg': 'image',
            'png': 'image',
            'gif': 'gif',
            'mp3': 'audio_file',
            'wav': 'audio_file',
            'mp4': 'video_file',
            'avi': 'video_file',
            'mov': 'video_file',
            'zip': 'folder_zip',
            'rar': 'folder_zip'
        };

        return iconMap[extension] || 'insert_drive_file';
    };

    onMount(async () => {
        console.log("FileTree component mounted, sharedMode:", sharedMode);

        await fetchUserGroups();
        await fetchData();

        const dropZone = document.querySelector('.file-tree-container');
        if (dropZone) {
            dropZone.addEventListener('dragover', (e) => {
                e.preventDefault();
                dragOver = true;
            });

            dropZone.addEventListener('dragleave', () => {
                dragOver = false;
            });

            dropZone.addEventListener('drop', async (e) => {
                e.preventDefault();
                dragOver = false;

                if (e.dataTransfer.files.length > 0) {
                    try {
                        isUploading = true;
                        const token = JSON.parse(localStorage.getItem('user')).token;
                        for (let i = 0; i < e.dataTransfer.files.length; i++) {
                            if (sharedMode && selectedGroupId) {
                                await uploadSharedFile(token, selectedGroupId, currentFolderId, e.dataTransfer.files[i]);
                            } else {
                                await uploadFile(token, currentFolderId, e.dataTransfer.files[i]);
                            }
                        }
                        success.set('Файлы успешно загружены');
                        setTimeout(() => success.set(null), 3000);
                        await fetchData();
                    } catch (err) {
                        error.set(err.message);
                        setTimeout(() => error.set(null), 3000);
                    } finally {
                        isUploading = false;
                    }
                }
            });
        }
    });

    const fetchUserGroups = async () => {
        try {
            const token = JSON.parse(localStorage.getItem('user')).token;
            const response = await getUserGroups(token);
            userGroups.set(response.groups || []);
            if (response.groups && response.groups.length > 0) {
                selectedGroupId = response.groups[0].group_id;
            }
        } catch (err) {
            console.error('Error fetching user groups:', err);
        }
    };

    const fetchData = async () => {
        loading.set(true);
        try {
            const token = JSON.parse(localStorage.getItem('user')).token;

            if (favoritesMode) {
                // Загружаем только избранное
                const favoritesResponse = await getAllFavorites(token);
                foldersList.set(favoritesResponse.folders || []);
                filesList.set(favoritesResponse.files || []);
            } else {
                console.log(`Fetching data for folder ID: ${currentFolderId}, sharedMode: ${sharedMode}`);

                const [folderResponse, fileResponse] = await Promise.all([
                    getFolders(token, currentFolderId),
                    getFileTree(token, currentFolderId)
                ]);

                let folders = folderResponse?.folders || [];
                let files = fileResponse?.files || [];

                // Фильтруем по режиму
                if (sharedMode) {
                    folders = folders.filter(folder => folder.folder_type === 'shared');
                    files = files.filter(file => file.file_type === 'shared');
                } else {
                    folders = folders.filter(folder => folder.folder_type === 'personal' || !folder.folder_type);
                    files = files.filter(file => file.file_type === 'personal' || !file.file_type);
                }

                foldersList.set(folders);
                filesList.set(files);
            }

            searchQuery.set('');
            selectedItems.set([]);

        } catch (err) {
            console.error("Error fetching data:", err);
            error.set(err.message);
            setTimeout(() => error.set(null), 3000);

            foldersList.set([]);
            filesList.set([]);
        } finally {
            loading.set(false);
        }
    };

    const handleUpload = async () => {
        const filesToUpload = uploadInput.files;
        if (filesToUpload.length === 0) {
            error.set('Файл не выбран');
            setTimeout(() => error.set(null), 3000);
            return;
        }

        isUploading = true;
        try {
            const token = JSON.parse(localStorage.getItem('user')).token;
            for (let i = 0; i < filesToUpload.length; i++) {
                if (sharedMode && selectedGroupId) {
                    await uploadSharedFile(token, selectedGroupId, currentFolderId, filesToUpload[i]);
                } else {
                    await uploadFile(token, currentFolderId, filesToUpload[i]);
                }
            }
            uploadInput.value = '';
            success.set('Файлы успешно загружены');
            setTimeout(() => success.set(null), 3000);
            await fetchData();
        } catch (err) {
            error.set(err.message);
            setTimeout(() => error.set(null), 3000);
        } finally {
            isUploading = false;
        }
    };

    const confirmDelete = () => {
        if ($selectedItems.length === 0) {
            error.set('Не выбраны элементы для удаления');
            setTimeout(() => error.set(null), 3000);
            return;
        }
        showDeleteConfirm = true;
    };

    const handleDelete = async () => {
        showDeleteConfirm = false;
        loading.set(true);
        try {
            const token = JSON.parse(localStorage.getItem('user')).token;
            const fileIds = $selectedItems.filter(item => item.type === 'file').map(item => item.id);
            const folderIds = $selectedItems.filter(item => item.type === 'folder').map(item => item.id);

            if (fileIds.length > 0) {
                await deleteFiles(token, fileIds);
            }
            if (folderIds.length > 0) {
                for (let folderId of folderIds) {
                    await deleteFolder(token, folderId);
                }
            }

            success.set('Элементы успешно удалены');
            setTimeout(() => success.set(null), 3000);
            selectedItems.set([]);
            await fetchData();
        } catch (err) {
            error.set(err.message);
            setTimeout(() => error.set(null), 3000);
        } finally {
            loading.set(false);
        }
    };

    const handleDownload = async (fileId, fileName) => {
        try {
            const token = JSON.parse(localStorage.getItem('user')).token;
            loading.set(true);
            const blob = await downloadFile(token, fileId);
            loading.set(false);
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = fileName;
            document.body.appendChild(a);
            a.click();
            a.remove();
            window.URL.revokeObjectURL(url);

            success.set(`Файл ${fileName} скачан`);
            setTimeout(() => success.set(null), 3000);
        } catch (err) {
            loading.set(false);
            error.set(err.message);
            setTimeout(() => error.set(null), 3000);
        }
    };

    const handleCreateFolder = async () => {
        if (newFolderName.trim() === '') {
            error.set('Имя папки не может быть пустым');
            setTimeout(() => error.set(null), 3000);
            return;
        }

        loading.set(true);
        try {
            const token = JSON.parse(localStorage.getItem('user')).token;
            if (sharedMode && selectedGroupId) {
                await createSharedFolder(token, newFolderName, selectedGroupId, currentFolderId);
            } else {
                await createFolder(token, newFolderName, currentFolderId);
            }
            newFolderName = '';
            showNewFolderDialog = false;
            success.set('Папка создана');
            setTimeout(() => success.set(null), 3000);
            await fetchData();
        } catch (err) {
            error.set(err.message);
            setTimeout(() => error.set(null), 3000);
        } finally {
            loading.set(false);
        }
    };

    const navigateToFolder = (folderId, folderName) => {
        if (favoritesMode) {
            error.set('Навигация по папкам недоступна в режиме избранного');
            setTimeout(() => error.set(null), 3000);
            return;
        }
        console.log(`Navigating to folder: ${folderName} (ID: ${folderId})`);
        folderStack.push({ id: currentFolderId, name: folderName });
        currentFolderId = folderId;
        fetchData();
    };

    const navigateBack = () => {
        if (folderStack.length > 0) {
            const previousFolder = folderStack.pop();
            console.log(`Navigating back to: ${previousFolder.name} (ID: ${previousFolder.id})`);
            currentFolderId = previousFolder.id;
            fetchData();
        }
    };

    const navigateHome = () => {
        console.log("Navigating to home");
        currentFolderId = 0;
        folderStack = [];
        fetchData();
    };

    const toggleItemSelection = (item, event) => {
        if (event) event.stopPropagation();
        const index = $selectedItems.findIndex(selected => selected.id === item.id && selected.type === item.type);
        if (index !== -1) {
            selectedItems.update(items => items.filter((_, i) => i !== index));
        } else {
            selectedItems.update(items => [...items, item]);
        }
    };

    const isItemSelected = (item) => {
        return $selectedItems.some(selected => selected.id === item.id && selected.type === item.type);
    };

    const toggleViewMode = () => {
        viewMode = viewMode === 'grid' ? 'list' : 'grid';
        localStorage.setItem('viewMode', viewMode);
    };

    const toggleSelectAll = () => {
        const folders = $filteredFolders;
        const files = $filteredFiles;
        const allItems = $selectedItems;

        if (allItems.length === folders.length + files.length) {
            selectedItems.set([]);
        } else {
            const newSelectedItems = [];
            folders.forEach(folder => {
                newSelectedItems.push({ id: folder.folder_id, type: 'folder', name: folder.folder_name });
            });
            files.forEach(file => {
                newSelectedItems.push({ id: file.file_id, type: 'file', name: file.file_name });
            });
            selectedItems.set(newSelectedItems);
        }
    };

    // Reactive statements
    $: totalItems = $filteredFolders.length + $filteredFiles.length;
    $: allSelected = $selectedItems.length === totalItems && totalItems > 0;
    $: selectedCount = $selectedItems.length;
    $: isEmptyContent = totalItems === 0;
    $: searchValue = $searchQuery;
    $: pageTitle = favoritesMode ? 'Избранное' : (sharedMode ? 'Общие файлы' : 'Мой диск');
    $: pageIcon = favoritesMode ? 'star' : (sharedMode ? 'group' : 'cloud');

    $: if (favoritesMode !== undefined) {
        currentFolderId = 0;
        folderStack = [];
        fetchData();
    }

    // Watch for sharedMode changes
    $: if (sharedMode !== undefined) {
        currentFolderId = 0;
        folderStack = [];
        fetchData();
    }
</script>

<div
        class="file-tree-container {dragOver ? 'drag-over' : ''}"
        class:loading={$loading}
>
    {#if $loading}
        <div class="loading-overlay">
            <div class="spinner"></div>
            <span>Загрузка...</span>
        </div>
    {/if}

    {#if dragOver}
        <div class="drop-zone-overlay">
            <div class="drop-zone-content">
                <i class="material-icons">cloud_upload</i>
                <span>Перетащите файлы сюда</span>
            </div>
        </div>
    {/if}

    <!-- Заголовок страницы -->
    <div class="page-header">
        <div class="page-title">
            <i class="material-icons">{pageIcon}</i>
            <h1>{pageTitle}</h1>
        </div>
        {#if sharedMode && $userGroups.length > 1}
            <div class="group-selector">
                <label for="group-select">Группа:</label>
                <select id="group-select" bind:value={selectedGroupId} on:change={fetchData}>
                    {#each $userGroups as group}
                        <option value={group.group_id}>{group.group_name}</option>
                    {/each}
                </select>
            </div>
        {/if}
    </div>

    <!-- Панель инструментов -->
    <div class="toolbar">
        <div class="toolbar-left">
            {#if !favoritesMode}
                <div class="navigation-buttons">
                    <button class="icon-button" on:click={navigateBack} disabled={folderStack.length === 0} title="Назад">
                        <i class="material-icons">arrow_back</i>
                    </button>
                    <button class="icon-button" on:click={navigateHome} disabled={currentFolderId === 0} title="Домой">
                        <i class="material-icons">home</i>
                    </button>
                </div>

                <div class="breadcrumb">
            <span class="breadcrumb-item" on:click={navigateHome} title={pageTitle}>
                <i class="material-icons">{pageIcon}</i>
                <span>{pageTitle}</span>
            </span>
                    {#each folderStack as folder, index}
                        <i class="material-icons separator">chevron_right</i>
                        <span class="breadcrumb-item" on:click={() => {
                    currentFolderId = folder.id;
                    folderStack = folderStack.slice(0, index + 1);
                    fetchData();
                }}>
                    {folder.name}
                </span>
                    {/each}
                </div>
            {:else}
                <div class="page-title-toolbar">
                    <i class="material-icons">{pageIcon}</i>
                    <span>{pageTitle}</span>
                </div>
            {/if}
        </div>

        <div class="toolbar-right">
            <div class="search-container">
                <i class="material-icons">search</i>
                <input
                        type="text"
                        bind:value={searchValue}
                        on:input={e => searchQuery.set(e.target.value)}
                        placeholder="Поиск"
                        class="search-input"
                />
                {#if searchValue}
                    <button class="clear-search" on:click={() => searchQuery.set('')}>
                        <i class="material-icons">close</i>
                    </button>
                {/if}
            </div>

            <button class="icon-button" on:click={toggleViewMode} title={viewMode === 'grid' ? 'Показать список' : 'Показать сетку'}>
                <i class="material-icons">{viewMode === 'grid' ? 'view_list' : 'grid_view'}</i>
            </button>
        </div>
    </div>

    <!-- Панель действий -->
    <div class="actions-bar">
        <div class="actions-left">
            {#if selectedCount > 0}
                <span class="selected-count">{selectedCount} {selectedCount === 1 ? 'элемент' : 'элемента'} выбрано</span>
                <button class="action-button" on:click={confirmDelete} title="Удалить выбранное">
                    <i class="material-icons">delete</i>
                    <span>Удалить</span>
                </button>

                <button class="action-button" on:click={openMoveDialog} title="Переместить выбранное">
                    <i class="material-icons">drive_file_move</i>
                    <span>Переместить</span>
                </button>

                {#if selectedCount === 1 && $selectedItems[0].type === 'file'}
                    <button class="action-button" on:click={() => handleDownload($selectedItems[0].id, $selectedItems[0].name)} title="Скачать">
                        <i class="material-icons">download</i>
                        <span>Скачать</span>
                    </button>
                {/if}
            {:else if !favoritesMode}
                <button class="action-button upload-button" title="Загрузить файлы">
                    <i class="material-icons">cloud_upload</i>
                    <span>Загрузить файлы</span>
                    <input
                            type="file"
                            bind:this={uploadInput}
                            on:change={handleUpload}
                            multiple
                            class="file-input"
                    />
                </button>
                <button class="action-button" on:click={() => showNewFolderDialog = true} title="Создать папку">
                    <i class="material-icons">create_new_folder</i>
                    <span>Создать папку</span>
                </button>
            {/if}
        </div>

        <div class="actions-right">
            {#if totalItems > 0}
                <button class="action-button" on:click={toggleSelectAll} title={allSelected ? 'Снять выделение' : 'Выбрать все'}>
                    <i class="material-icons">{allSelected ? 'deselect' : 'select_all'}</i>
                    <span>{allSelected ? 'Снять выделение' : 'Выбрать все'}</span>
                </button>
            {/if}
        </div>
    </div>

    <!-- Сообщения об ошибках и успешных действиях -->
    {#if $error}
        <div class="message error-message">
            <i class="material-icons">error</i>
            <span>{$error}</span>
            <button class="close-button" on:click={() => error.set(null)}>
                <i class="material-icons">close</i>
            </button>
        </div>
    {/if}

    {#if $success}
        <div class="message success-message">
            <i class="material-icons">check_circle</i>
            <span>{$success}</span>
            <button class="close-button" on:click={() => success.set(null)}>
                <i class="material-icons">close</i>
            </button>
        </div>
    {/if}

    <!-- Содержимое папки -->
    <div class="content-container {viewMode}">
        {#if isEmptyContent}
            <div class="empty-folder">
                <i class="material-icons">folder_open</i>
                {#if searchValue}
                    <p>По запросу "{searchValue}" ничего не найдено</p>
                {:else}
                    <p>Эта папка пуста</p>
                {/if}
                <p class="empty-hint">Перетащите файлы сюда или воспользуйтесь кнопкой "Загрузить файлы"</p>
            </div>
        {:else}
            <!-- Заголовки списка, если выбран режим list -->
            {#if viewMode === 'list' && totalItems > 0}
                <div class="list-header">
                    <div class="list-cell checkbox-cell">
                        <input
                                type="checkbox"
                                checked={allSelected}
                                on:change={toggleSelectAll}
                        />
                    </div>
                    <div class="list-cell name-cell">Название</div>
                    <div class="list-cell date-cell">Дата создания</div>
                    <div class="list-cell size-cell">Размер</div>
                    <div class="list-cell actions-cell"></div>
                </div>
            {/if}

            {#if viewMode === 'grid'}
                <div class="grid-container">
                    <!-- Папки -->
                    {#each $filteredFolders as folder (folder.folder_id)}
                        <div
                                class="grid-item folder-item {isItemSelected({id: folder.folder_id, type: 'folder'}) ? 'selected' : ''}"
                                on:click={() => navigateToFolder(folder.folder_id, folder.folder_name)}
                        >
                            <div class="item-checkbox" on:click|stopPropagation>
                                <input
                                        type="checkbox"
                                        checked={isItemSelected({id: folder.folder_id, type: 'folder'})}
                                        on:change={(e) => toggleItemSelection({id: folder.folder_id, type: 'folder', name: folder.folder_name}, e)}
                                />
                            </div>
                            <button
                                    class="favorite-button {folder.is_favorite ? 'active' : ''}"
                                    on:click={(e) => toggleFavorite(folder, 'folder', e)}
                                    title={folder.is_favorite ? 'Удалить из избранного' : 'Добавить в избранное'}
                            >
                                <i class="material-icons">{folder.is_favorite ? 'star' : 'star_border'}</i>
                            </button>
                            {#if folder.folder_type === 'shared'}
                                <div class="shared-indicator" title="Общая папка">
                                    <i class="material-icons">group</i>
                                </div>
                            {/if}
                            <div class="item-icon">
                                <i class="material-icons">{folder.folder_type === 'shared' ? 'folder_shared' : 'folder'}</i>
                            </div>
                            <div class="item-details">
                                <div class="item-name" title={folder.folder_name}>{folder.folder_name}</div>
                                <div class="item-meta">{formatDate(folder.created_at)}</div>
                            </div>
                        </div>
                    {/each}

                    <!-- Файлы -->
                    {#each $filteredFiles as file (file.file_id)}
                        <div
                                class="grid-item file-item {isItemSelected({id: file.file_id, type: 'file'}) ? 'selected' : ''}"
                                on:click={() => handleDownload(file.file_id, file.file_name)}
                        >
                            <div class="item-checkbox" on:click|stopPropagation>
                                <input
                                        type="checkbox"
                                        checked={isItemSelected({id: file.file_id, type: 'file'})}
                                        on:change={(e) => toggleItemSelection({id: file.file_id, type: 'file', name: file.file_name}, e)}
                                />
                            </div>
                            <button
                                    class="favorite-button {file.is_favorite ? 'active' : ''}"
                                    on:click={(e) => toggleFavorite(file, 'file', e)}
                                    title={file.is_favorite ? 'Удалить из избранного' : 'Добавить в избранное'}
                            >
                                <i class="material-icons">{file.is_favorite ? 'star' : 'star_border'}</i>
                            </button>
                            {#if file.file_type === 'shared'}
                                <div class="shared-indicator" title="Общий файл">
                                    <i class="material-icons">group</i>
                                </div>
                            {/if}
                            <div class="item-icon">
                                <i class="material-icons">{getFileIcon(file.file_name)}</i>
                            </div>
                            <div class="item-details">
                                <div class="item-name" title={file.file_name}>{file.file_name}</div>
                                <div class="item-meta">
                                    {formatFileSize(file.file_size)} • {formatDate(file.created_at)}
                                </div>
                            </div>
                        </div>
                    {/each}
                </div>
            {:else}
                <!-- Список папок -->
                {#each $filteredFolders as folder (folder.folder_id)}
                    <div
                            class="list-row folder-item {isItemSelected({id: folder.folder_id, type: 'folder'}) ? 'selected' : ''}"
                            on:click={() => navigateToFolder(folder.folder_id, folder.folder_name)}
                    >
                        <div class="list-cell checkbox-cell" on:click|stopPropagation>
                            <input
                                    type="checkbox"
                                    checked={isItemSelected({id: folder.folder_id, type: 'folder'})}
                                    on:change={(e) => toggleItemSelection({id: folder.folder_id, type: 'folder', name: folder.folder_name}, e)}
                            />
                        </div>
                        <div class="list-cell name-cell">
                            <i class="material-icons item-type-icon">{folder.folder_type === 'shared' ? 'folder_shared' : 'folder'}</i>
                            <span>{folder.folder_name}</span>
                            {#if folder.folder_type === 'shared'}
                                <span class="shared-badge">
                                    <i class="material-icons tiny">group</i>
                                    Общая
                                </span>
                            {/if}
                        </div>
                        <div class="list-cell date-cell">{formatDate(folder.created_at)}</div>
                        <div class="list-cell size-cell">—</div>
                        <div class="list-cell actions-cell">
                            <button class="icon-button" on:click|stopPropagation={() => toggleItemSelection({id: folder.folder_id, type: 'folder', name: folder.folder_name})}>
                                <i class="material-icons">more_vert</i>
                            </button>
                        </div>
                    </div>
                {/each}

                <!-- Список файлов -->
                {#each $filteredFiles as file (file.file_id)}
                    <div
                            class="list-row file-item {isItemSelected({id: file.file_id, type: 'file'}) ? 'selected' : ''}"
                            on:click={() => handleDownload(file.file_id, file.file_name)}
                    >
                        <div class="list-cell checkbox-cell" on:click|stopPropagation>
                            <input
                                    type="checkbox"
                                    checked={isItemSelected({id: file.file_id, type: 'file'})}
                                    on:change={(e) => toggleItemSelection({id: file.file_id, type: 'file', name: file.file_name}, e)}
                            />
                        </div>
                        <div class="list-cell name-cell">
                            <i class="material-icons item-type-icon">{getFileIcon(file.file_name)}</i>
                            <span>{file.file_name}</span>
                            {#if file.file_type === 'shared'}
                                <span class="shared-badge">
                                    <i class="material-icons tiny">group</i>
                                    Общий
                                </span>
                            {/if}
                        </div>
                        <div class="list-cell date-cell">{formatDate(file.created_at)}</div>
                        <div class="list-cell size-cell">{formatFileSize(file.file_size)}</div>
                        <div class="list-cell actions-cell">
                            <button class="icon-button" on:click|stopPropagation={() => handleDownload(file.file_id, file.file_name)}>
                                <i class="material-icons">download</i>
                            </button>
                        </div>
                    </div>
                {/each}
            {/if}
        {/if}
    </div>
</div>

{#if showMoveDialog}
    <div class="modal-overlay">
        <div class="modal-dialog">
            <div class="modal-header">
                <h3>Переместить {selectedCount} {selectedCount === 1 ? 'файл' : 'файла'}</h3>
                <button class="close-button" on:click={() => showMoveDialog = false}>
                    <i class="material-icons">close</i>
                </button>
            </div>
            <div class="modal-body">
                <p>Выберите папку назначения:</p>

                {#if isFoldersLoading}
                    <div class="folder-loading">
                        <div class="spinner small"></div>
                        <span>Загрузка папок...</span>
                    </div>
                {:else}
                    <div class="folder-list">
                        <div class="folder-item {targetFolderId === 0 ? 'selected' : ''}"
                             on:click={() => targetFolderId = 0}>
                            <i class="material-icons">cloud</i>
                            <span>Корневая папка</span>
                        </div>

                        {#each $allFolders as folder}
                            <div class="folder-item {targetFolderId === folder.folder_id ? 'selected' : ''}"
                                 on:click={() => targetFolderId = folder.folder_id}>
                                <i class="material-icons">folder</i>
                                <span>{folder.folder_name}</span>
                            </div>

                            {#if folder.subFolders && folder.subFolders.length > 0}
                                {#each folder.subFolders as subFolder}
                                    <div class="folder-item subfolder {targetFolderId === subFolder.folder_id ? 'selected' : ''}"
                                         on:click={() => targetFolderId = subFolder.folder_id}>
                                        <i class="material-icons">subdirectory_arrow_right</i>
                                        <i class="material-icons">folder</i>
                                        <span>{subFolder.folder_name}</span>
                                    </div>
                                {/each}
                            {/if}
                        {/each}
                    </div>
                {/if}
            </div>
            <div class="modal-footer">
                <button class="button secondary" on:click={() => showMoveDialog = false}>Отмена</button>
                <button class="button primary" on:click={handleMove} disabled={isFoldersLoading}>Переместить</button>
            </div>
        </div>
    </div>
{/if}

<!-- Модальное окно для создания новой папки -->
{#if showNewFolderDialog}
    <div class="modal-overlay">
        <div class="modal-dialog">
            <div class="modal-header">
                <h3>Создать {sharedMode && selectedGroupId ? 'общую' : 'новую'} папку</h3>
                <button class="close-button" on:click={() => showNewFolderDialog = false}>
                    <i class="material-icons">close</i>
                </button>
            </div>
            <div class="modal-body">
                {#if sharedMode && selectedGroupId}
                    <p class="shared-mode-info">
                        <i class="material-icons">info</i>
                        Папка будет создана для группы: <strong>{$userGroups.find(g => g.group_id === selectedGroupId)?.group_name}</strong>
                    </p>
                {/if}
                <div class="form-group">
                    <label for="new-folder-name">Название папки</label>
                    <input
                            type="text"
                            id="new-folder-name"
                            bind:value={newFolderName}
                            placeholder="Введите название папки"
                            autofocus
                    />
                </div>
            </div>
            <div class="modal-footer">
                <button class="button secondary" on:click={() => showNewFolderDialog = false}>Отмена</button>
                <button class="button primary" on:click={handleCreateFolder} disabled={!newFolderName.trim()}>Создать</button>
            </div>
        </div>
    </div>
{/if}

<!-- Модальное окно подтверждения удаления -->
{#if showDeleteConfirm}
    <div class="modal-overlay">
        <div class="modal-dialog">
            <div class="modal-header">
                <h3>Подтверждение удаления</h3>
                <button class="close-button" on:click={() => showDeleteConfirm = false}>
                    <i class="material-icons">close</i>
                </button>
            </div>
            <div class="modal-body">
                <p class="delete-message">
                    <i class="material-icons warning-icon">warning</i>
                    Вы действительно хотите удалить выбранные элементы ({selectedCount})?
                </p>
                <p class="delete-warning">Это действие нельзя отменить!</p>
            </div>
            <div class="modal-footer">
                <button class="button secondary" on:click={() => showDeleteConfirm = false}>Отмена</button>
                <button class="button danger" on:click={handleDelete}>Удалить</button>
            </div>
        </div>
    </div>
{/if}

<style>
    /* Подключаем иконки Material Icons */
    @import url('https://fonts.googleapis.com/icon?family=Material+Icons');

    /* Основные стили контейнера */
    .file-tree-container {
        display: flex;
        flex-direction: column;
        background-color: var(--bg-secondary, #f5f8fa);
        border-radius: 8px;
        box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
        overflow: hidden;
        position: relative;
        min-height: 500px;
        transition: all 0.3s ease;
    }

    .file-tree-container.loading {
        opacity: 0.8;
    }

    /* Стили для зоны перетаскивания */
    .file-tree-container.drag-over {
        border: 2px dashed var(--accent-color, #1a73e8);
        background-color: rgba(26, 115, 232, 0.05);
    }

    .drop-zone-overlay {
        position: absolute;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: rgba(255, 255, 255, 0.9);
        display: flex;
        align-items: center;
        justify-content: center;
        z-index: 10;
    }

    .drop-zone-content {
        display: flex;
        flex-direction: column;
        align-items: center;
        color: var(--accent-color, #1a73e8);
    }

    .drop-zone-content i {
        font-size: 48px;
        margin-bottom: 10px;
    }

    /* Заголовок страницы */
    .page-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 16px 16px 0 16px;
        margin-bottom: 16px;
    }

    .page-title {
        display: flex;
        align-items: center;
    }

    .page-title i {
        margin-right: 12px;
        font-size: 28px;
        color: var(--accent-color, #1a73e8);
    }

    .page-title h1 {
        margin: 0;
        font-size: 24px;
        font-weight: 500;
        color: var(--text-primary, #202124);
    }

    .group-selector {
        display: flex;
        align-items: center;
        gap: 8px;
    }

    .group-selector label {
        font-size: 14px;
        color: var(--text-secondary, #5f6368);
    }

    .group-selector select {
        padding: 6px 12px;
        border: 1px solid var(--border-color, #dadce0);
        border-radius: 4px;
        background-color: var(--bg-primary, #ffffff);
        font-size: 14px;
        color: var(--text-primary, #202124);
        cursor: pointer;
    }

    /* Стили панели инструментов */
    .toolbar {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 10px 16px;
        background-color: var(--bg-primary, #ffffff);
        border-bottom: 1px solid var(--border-color, #e0e0e0);
    }

    .toolbar-left, .toolbar-right {
        display: flex;
        align-items: center;
    }

    .navigation-buttons {
        display: flex;
        margin-right: 16px;
    }

    .icon-button {
        background: none;
        border: none;
        color: var(--text-secondary, #5f6368);
        width: 36px;
        height: 36px;
        border-radius: 50%;
        display: flex;
        align-items: center;
        justify-content: center;
        cursor: pointer;
        transition: background-color 0.2s;
        padding: 0;
    }

    .icon-button:hover:not(:disabled) {
        background-color: var(--hover-bg, #f1f3f4);
    }

    .icon-button:disabled {
        opacity: 0.5;
        cursor: not-allowed;
    }

    .breadcrumb {
        display: flex;
        align-items: center;
        font-size: 14px;
        flex-wrap: wrap;
    }

    .breadcrumb-item {
        display: flex;
        align-items: center;
        color: var(--text-primary, #202124);
        cursor: pointer;
        padding: 4px 8px;
        border-radius: 4px;
        transition: background-color 0.2s;
    }

    .breadcrumb-item:hover {
        background-color: var(--hover-bg, #f1f3f4);
    }

    .breadcrumb-item i {
        margin-right: 4px;
        font-size: 18px;
        color: var(--accent-color, #1a73e8);
    }

    .separator {
        font-size: 18px;
        color: var(--text-secondary, #5f6368);
        margin: 0 4px;
    }

    .search-container {
        display: flex;
        align-items: center;
        background-color: var(--bg-tertiary, #eef2f6);
        border-radius: 24px;
        padding: 0 8px 0 12px;
        margin-right: 12px;
        transition: background-color 0.2s, box-shadow 0.2s;
    }

    .search-container:focus-within {
        background-color: var(--bg-primary, #ffffff);
        box-shadow: 0 1px 6px rgba(32, 33, 36, 0.28);
    }

    .search-container i {
        color: var(--text-secondary, #5f6368);
        margin-right: 8px;
    }

    .search-input {
        border: none;
        background: transparent;
        padding: 8px 0;
        width: 200px;
        font-size: 14px;
        outline: none;
        color: var(--text-primary, #202124);
    }

    .clear-search {
        background: none;
        border: none;
        color: var(--text-secondary, #5f6368);
        cursor: pointer;
        padding: 4px;
        display: flex;
        align-items: center;
        justify-content: center;
    }
    /* Панель действий */
    .actions-bar {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 8px 16px;
        background-color: var(--bg-primary, #ffffff);
        border-bottom: 1px solid var(--border-color, #e0e0e0);
    }

    .actions-left, .actions-right {
        display: flex;
        align-items: center;
    }

    .action-button {
        display: flex;
        align-items: center;
        background-color: transparent;
        border: none;
        color: var(--text-primary, #202124);
        font-size: 14px;
        padding: 8px 12px;
        border-radius: 4px;
        cursor: pointer;
        margin-right: 8px;
        transition: background-color 0.2s;
    }

    .action-button:hover {
        background-color: var(--hover-bg, #f1f3f4);
    }

    .action-button i {
        margin-right: 8px;
        font-size: 20px;
    }

    .upload-button {
        position: relative;
        background-color: var(--accent-color, #1a73e8);
        color: white;
    }

    .upload-button:hover {
        background-color: var(--accent-hover, #1565c0);
    }

    .file-input {
        position: absolute;
        top: 0;
        left: 0;
        width: 100%;
        height: 100%;
        opacity: 0;
        cursor: pointer;
    }

    .selected-count {
        font-size: 14px;
        margin-right: 16px;
        color: var(--text-primary, #202124);
    }

    /* Сообщения */
    .message {
        display: flex;
        align-items: center;
        margin: 8px 16px;
        padding: 12px 16px;
        border-radius: 4px;
        font-size: 14px;
    }

    .message i {
        margin-right: 8px;
        font-size: 20px;
    }

    .error-message {
        background-color: #fdeded;
        color: #d93025;
        border-left: 4px solid #d93025;
    }

    .success-message {
        background-color: #e6f4ea;
        color: #1e8e3e;
        border-left: 4px solid #1e8e3e;
    }

    .message .close-button {
        margin-left: auto;
        background: none;
        border: none;
        color: inherit;
        cursor: pointer;
        padding: 4px;
    }

    /* Содержимое папки */
    .content-container {
        flex: 1;
        padding: 16px;
        overflow-y: auto;
    }

    /* Пустая папка */
    .empty-folder {
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        height: 100%;
        min-height: 200px;
        color: var(--text-secondary, #5f6368);
        padding: 32px;
        text-align: center;
    }

    .empty-folder i {
        font-size: 64px;
        margin-bottom: 16px;
        color: var(--text-secondary, #5f6368);
        opacity: 0.7;
    }

    .empty-folder p {
        margin: 8px 0;
        font-size: 16px;
    }

    .empty-hint {
        font-size: 14px;
        opacity: 0.8;
    }

    /* Сетка */
    .grid-container {
        display: grid;
        grid-template-columns: repeat(auto-fill, minmax(180px, 1fr));
        grid-gap: 16px;
        padding: 8px;
    }

    .grid-item {
        background-color: var(--bg-primary, #ffffff);
        border-radius: 8px;
        padding: 16px;
        position: relative;
        cursor: pointer;
        transition: box-shadow 0.2s, transform 0.1s;
        display: flex;
        flex-direction: column;
        align-items: center;
        box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
    }

    .grid-item:hover {
        box-shadow: 0 1px 6px rgba(32, 33, 36, 0.28);
    }

    .grid-item:active {
        transform: scale(0.98);
    }

    .item-checkbox {
        position: absolute;
        top: 8px;
        left: 8px;
        opacity: 0;
        transition: opacity 0.2s;
    }

    .grid-item:hover .item-checkbox,
    .grid-item.selected .item-checkbox {
        opacity: 1;
    }

    .shared-indicator {
        position: absolute;
        top: 8px;
        right: 8px;
        width: 24px;
        height: 24px;
        background-color: #34a853;
        color: white;
        border-radius: 50%;
        display: flex;
        align-items: center;
        justify-content: center;
    }

    .shared-indicator i {
        font-size: 14px;
    }

    .item-icon {
        width: 64px;
        height: 64px;
        display: flex;
        align-items: center;
        justify-content: center;
        margin-bottom: 12px;
    }

    .item-icon i {
        font-size: 40px;
    }

    .folder-item .item-icon i {
        color: #fbc02d;
    }

    .file-item .item-icon i {
        color: #4285f4;
    }

    .item-details {
        width: 100%;
        text-align: center;
    }

    .item-name {
        font-size: 14px;
        font-weight: 500;
        color: var(--text-primary, #202124);
        margin-bottom: 4px;
        white-space: nowrap;
        overflow: hidden;
        text-overflow: ellipsis;
    }

    .item-meta {
        font-size: 12px;
        color: var(--text-secondary, #5f6368);
    }
    /* Список */
    .content-container.list {
        padding: 0;
    }

    .list-header {
        display: flex;
        align-items: center;
        padding: 0 16px;
        height: 48px;
        background-color: var(--bg-tertiary, #eef2f6);
        border-bottom: 1px solid var(--border-color, #e0e0e0);
        font-size: 12px;
        font-weight: 500;
        color: var(--text-secondary, #5f6368);
        text-transform: uppercase;
    }

    .list-row {
        display: flex;
        align-items: center;
        padding: 0 16px;
        height: 56px;
        border-bottom: 1px solid var(--border-color, #e0e0e0);
        cursor: pointer;
        transition: background-color 0.2s;
    }

    .list-row:hover {
        background-color: var(--hover-bg, #f1f3f4);
    }

    .list-row.selected {
        background-color: rgba(26, 115, 232, 0.08);
    }

    .list-cell {
        display: flex;
        align-items: center;
    }

    .checkbox-cell {
        width: 40px;
        flex-shrink: 0;
    }

    .name-cell {
        flex: 2;
        min-width: 200px;
    }

    .date-cell {
        flex: 1;
        min-width: 140px;
    }

    .size-cell {
        flex: 1;
        min-width: 100px;
    }

    .actions-cell {
        width: 40px;
        flex-shrink: 0;
    }

    .item-type-icon {
        margin-right: 12px;
    }

    .folder-item .item-type-icon {
        color: #fbc02d;
    }

    .file-item .item-type-icon {
        color: #4285f4;
    }

    .shared-badge {
        display: inline-flex;
        align-items: center;
        gap: 4px;
        margin-left: 8px;
        padding: 2px 8px;
        background-color: #34a853;
        color: white;
        border-radius: 12px;
        font-size: 11px;
    }

    .folder-list {
        max-height: 300px;
        overflow-y: auto;
        border: 1px solid var(--border-color, #e0e0e0);
        border-radius: 4px;
        margin-top: 12px;
    }

    .folder-item {
        display: flex;
        align-items: center;
        padding: 10px 12px;
        cursor: pointer;
        transition: background-color 0.2s;
        border-bottom: 1px solid var(--border-color, #e0e0e0);
    }

    .folder-item:last-child {
        border-bottom: none;
    }

    .folder-item:hover {
        background-color: var(--hover-bg, #f1f3f4);
    }

    .folder-item.selected {
        background-color: rgba(26, 115, 232, 0.08);
    }

    .folder-item i {
        margin-right: 8px;
        color: #fbc02d;
    }

    .folder-item.subfolder {
        padding-left: 36px;
    }

    .folder-loading {
        display: flex;
        flex-direction: column;
        align-items: center;
        padding: 24px;
    }

    .spinner.small {
        width: 24px;
        height: 24px;
        margin-bottom: 12px;
    }

    /* Модальные окна */
    .modal-overlay {
        position: fixed;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: rgba(0, 0, 0, 0.5);
        display: flex;
        align-items: center;
        justify-content: center;
        z-index: 100;
    }

    .modal-dialog {
        background-color: white;
        border-radius: 8px;
        box-shadow: 0 24px 38px 3px rgba(0, 0, 0, 0.14),
        0 9px 46px 8px rgba(0, 0, 0, 0.12),
        0 11px 15px -7px rgba(0, 0, 0, 0.2);
        width: 400px;
        max-width: 90vw;
        overflow: hidden;
    }
    .modal-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 16px 24px;
        border-bottom: 1px solid var(--border-color, #e0e0e0);
    }

    .modal-header h3 {
        margin: 0;
        font-size: 18px;
        color: var(--text-primary, #202124);
    }

    .modal-body {
        padding: 24px;
    }

    .modal-footer {
        display: flex;
        justify-content: flex-end;
        padding: 16px 24px;
        border-top: 1px solid var(--border-color, #e0e0e0);
    }

    .form-group {
        margin-bottom: 16px;
    }

    .form-group label {
        display: block;
        margin-bottom: 8px;
        font-size: 14px;
        color: var(--text-secondary, #5f6368);
    }

    .form-group input {
        width: 100%;
        padding: 10px 12px;
        font-size: 14px;
        border: 1px solid var(--border-color, #e0e0e0);
        border-radius: 4px;
        outline: none;
        transition: border-color 0.2s, box-shadow 0.2s;
    }

    .form-group input:focus {
        border-color: var(--accent-color, #1a73e8);
        box-shadow: 0 0 0 2px rgba(26, 115, 232, 0.2);
    }

    .button {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        padding: 8px 16px;
        font-size: 14px;
        border-radius: 4px;
        cursor: pointer;
        border: none;
        font-weight: 500;
        margin-left: 8px;
        transition: background-color 0.2s;
    }

    .primary {
        background-color: var(--accent-color, #1a73e8);
        color: white;
    }

    .primary:hover {
        background-color: var(--accent-hover, #1565c0);
    }

    .secondary {
        background-color: var(--hover-bg, #f1f3f4);
        color: var(--text-primary, #202124);
    }

    .secondary:hover {
        background-color: #e8eaed;
    }

    .danger {
        background-color: #d93025;
        color: white;
    }

    .danger:hover {
        background-color: #c5221f;
    }

    .button:disabled {
        opacity: 0.6;
        cursor: not-allowed;
    }

    .delete-message {
        display: flex;
        align-items: center;
        font-size: 16px;
        margin-bottom: 16px;
    }

    .warning-icon {
        color: #f29900;
        font-size: 24px;
        margin-right: 12px;
    }

    .delete-warning {
        color: #d93025;
        font-weight: 500;
        font-size: 14px;
    }

    .shared-mode-info {
        display: flex;
        align-items: center;
        background-color: #e6f4ea;
        color: #1e8e3e;
        padding: 12px;
        border-radius: 4px;
        margin-bottom: 16px;
        font-size: 14px;
    }

    .shared-mode-info i {
        margin-right: 8px;
    }

    .shared-mode-info strong {
        font-weight: 600;
    }

    /* Индикатор загрузки */
    .loading-overlay {
        position: absolute;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: rgba(255, 255, 255, 0.7);
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        z-index: 1000;
    }

    .spinner {
        width: 40px;
        height: 40px;
        border-radius: 50%;
        border: 3px solid var(--accent-color, #1a73e8);
        border-top-color: transparent;
        animation: spin 1s linear infinite;
        margin-bottom: 16px;
    }

    @keyframes spin {
        to {
            transform: rotate(360deg);
        }
    }

    .loading-overlay span {
        font-size: 16px;
        color: var(--text-primary, #202124);
    }

    i.tiny {
        font-size: 14px !important;
    }

    /* Кнопка избранного */
    .favorite-button {
        position: absolute;
        top: 8px;
        right: 8px;
        width: 28px;
        height: 28px;
        background-color: transparent;
        border: none;
        border-radius: 50%;
        display: flex;
        align-items: center;
        justify-content: center;
        cursor: pointer;
        opacity: 0;
        transition: opacity 0.2s, transform 0.1s;
        z-index: 10;
    }

    .favorite-button:hover {
        transform: scale(1.1);
    }

    .favorite-button.active {
        opacity: 1;
    }

    .grid-item:hover .favorite-button {
        opacity: 1;
    }

    .favorite-button i {
        font-size: 20px;
        color: #9aa0a6;
        text-shadow: 0 1px 2px rgba(0, 0, 0, 0.3);
    }

    .favorite-button.active i {
        color: #fbc02d;
    }

    .favorite-button:hover i {
        color: #fbc02d;
    }

    /* Адаптивность */
    @media (max-width: 768px) {
        .grid-container {
            grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
        }

        .toolbar {
            flex-direction: column;
            align-items: stretch;
        }

        .toolbar-left, .toolbar-right {
            width: 100%;
        }

        .toolbar-right {
            margin-top: 12px;
        }

        .search-container {
            flex: 1;
        }

        .search-input {
            width: 100%;
        }

        .actions-bar {
            flex-wrap: wrap;
        }

        .actions-right {
            margin-top: 8px;
        }

        .breadcrumb {
            overflow-x: auto;
            white-space: nowrap;
            max-width: 100%;
            padding-bottom: 4px;
        }

        .list-header, .list-row {
            padding: 0 8px;
        }

        .date-cell, .size-cell {
            display: none;
        }
        .page-title-toolbar {
            display: flex;
            align-items: center;
            font-size: 18px;
            font-weight: 500;
            color: var(--text-primary, #202124);
        }

        .page-title-toolbar i {
            margin-right: 12px;
            color: var(--accent-color, #1a73e8);
            font-size: 24px;
        }
    }
</style>